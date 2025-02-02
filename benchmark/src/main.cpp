#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include <array_trie.hpp>
#include <hash_trie.hpp>
#include <vector_trie.hpp>

#if defined(__GNUC__) || defined(__clang__)
template<typename T>
inline void
DoNotOptimize(T const& value)
{
  asm volatile("" : : "r,m"(value) : "memory");
}
#else
template<typename T>
inline void
DoNotOptimize(T const& value)
{
  volatile auto dummy = value;
  (void)dummy;
}
#endif

static std::string
random_word(std::mt19937& rng, int min_word_length, int max_word_length)
{
  static constexpr char chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  std::uniform_int_distribution length_dist(static_cast<std::size_t>(min_word_length), static_cast<std::size_t>(max_word_length));
  std::uniform_int_distribution<> chars_dist(0, sizeof(chars) - 2);
  const auto length = length_dist(rng);
  std::string result;
  result.reserve((length + 1));
  for (std::size_t i = 0; i < length; ++i)
    result.push_back(chars[chars_dist(rng)]);
  result.push_back('$');
  return result;
}

struct Instance
{
  int num_words;
  int min_word_length;
  int max_word_length;
  int num_insert_queries;
  int num_contains_queries;
  int num_remove_queries;
  int chance_random_query;
  std::vector<std::string> words{};
  std::vector<std::pair<int, std::string>> queries{};
};

Instance
create_instance(int num_words,
                int min_word_length,
                int max_word_length,
                int num_insert_queries,
                int num_contains_queries,
                int num_remove_queries,
                int chance_random_query)
{
  std::random_device rd;
  std::mt19937 rng(rd());

  Instance instance{ num_words, min_word_length, max_word_length, num_insert_queries, num_contains_queries, num_remove_queries, chance_random_query };

  instance.words.reserve(static_cast<std::size_t>(num_words));
  for (int i = 0; i < num_words; ++i)
    instance.words.push_back(random_word(rng, min_word_length, max_word_length));

  std::uniform_int_distribution percent_dist(0, 99);
  std::uniform_int_distribution<std::size_t> index_dist(0, static_cast<std::size_t>(num_words - 1));
  instance.queries.reserve(static_cast<std::size_t>(num_insert_queries + num_remove_queries + num_contains_queries));

  // Insert queries.
  for (int i = 0; i < num_insert_queries; ++i) {
    std::string word = (percent_dist(rng) < chance_random_query) ? random_word(rng, min_word_length, max_word_length) : instance.words[index_dist(rng)];
    instance.queries.emplace_back(0, std::move(word));
  }
  // Remove queries.
  for (int i = 0; i < num_remove_queries; ++i) {
    std::string word = (percent_dist(rng) < chance_random_query) ? random_word(rng, min_word_length, max_word_length) : instance.words[index_dist(rng)];
    instance.queries.emplace_back(1, std::move(word));
  }
  // Contains queries.
  for (int i = 0; i < num_contains_queries; ++i) {
    std::string word = (percent_dist(rng) < chance_random_query) ? random_word(rng, min_word_length, max_word_length) : instance.words[index_dist(rng)];
    instance.queries.emplace_back(2, std::move(word));
  }
  std::shuffle(instance.queries.begin(), instance.queries.end(), rng);

  return instance;
}

struct BenchmarkResult
{
  std::string variant;
  long construction_time; // in nanoseconds
  long query_time;        // in nanoseconds
  std::size_t final_size;
};

template<typename Trie>
BenchmarkResult
run_benchmark_instance(const Instance& instance, const std::string& variant_name)
{
  Trie trie;

  // --- Construction Phase ---
  const auto start_construction = std::chrono::steady_clock::now();
  for (const auto& word : instance.words)
    trie.insert(word);
  const auto end_construction = std::chrono::steady_clock::now();
  const auto construction_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_construction - start_construction).count();
  const auto final_size = trie.size();

  // --- Query Phase ---
  volatile int dummy_accum = 0;
  const auto start_query = std::chrono::steady_clock::now();
  for (const auto& [op, word] : instance.queries) {
    switch (op) {
      case 0:
        dummy_accum ^= static_cast<int>(trie.insert(word));
        break;
      case 1:
        dummy_accum ^= static_cast<int>(trie.remove(word));
        break;
      case 2:
        dummy_accum ^= static_cast<int>(trie.contains(word));
        break;
      default:
        break;
    }
  }
  DoNotOptimize(dummy_accum);
  const auto end_query = std::chrono::steady_clock::now();
  const auto query_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_query - start_query).count();

  return { variant_name, construction_time, query_time, final_size };
}

template<typename Trie>
BenchmarkResult
run_benchmark_average(const Instance& instance, const std::string& variant_name, int runs)
{
  long total_construction = 0;
  long total_query = 0;
  std::size_t total_size = 0;
  for (int i = 0; i < runs; ++i) {
    BenchmarkResult res = run_benchmark_instance<Trie>(instance, variant_name);
    total_construction += res.construction_time;
    total_query += res.query_time;
    total_size += res.final_size;
  }
  return { variant_name,
           total_construction / static_cast<decltype(total_construction)>(runs),
           total_query / static_cast<decltype(total_query)>(runs),
           total_size / static_cast<decltype(total_size)>(runs) };
}

void
plot_fill_factor()
{
  const auto num_words_vec = std::vector{ 25'000, 50'000, 100'000, 200'000, 400'000 };
  const auto min_word_length = 4, max_word_length = 24;
  const auto chance_random_query = 50;
  const auto runs = 5;

  std::ofstream ofs;

  ofs = std::ofstream("plot_fill_factor_insert.csv");
  ofs << "num_words,variant,query_time_ns\n";
  for (const auto num_words : num_words_vec) {
    Instance instance = create_instance(num_words, min_word_length, max_word_length, 100000, 0, 0, chance_random_query);
    BenchmarkResult vec = run_benchmark_average<VectorTrie>(instance, "VectorTrie", runs);
    BenchmarkResult arr = run_benchmark_average<ArrayTrie>(instance, "ArrayTrie", runs);
    BenchmarkResult hash = run_benchmark_average<HashTrie>(instance, "HashTrie", runs);
    ofs << num_words << ",VectorTrie," << vec.query_time << "\n";
    ofs << num_words << ",ArrayTrie," << arr.query_time << "\n";
    ofs << num_words << ",HashTrie," << hash.query_time << "\n";
  }

  ofs = std::ofstream("plot_fill_factor_contains.csv");
  ofs << "num_words,variant,query_time_ns\n";
  for (const auto num_words : num_words_vec) {
    Instance instance = create_instance(num_words, min_word_length, max_word_length, 0, 100000, 0, chance_random_query);
    BenchmarkResult vec = run_benchmark_average<VectorTrie>(instance, "VectorTrie", runs);
    BenchmarkResult arr = run_benchmark_average<ArrayTrie>(instance, "ArrayTrie", runs);
    BenchmarkResult hash = run_benchmark_average<HashTrie>(instance, "HashTrie", runs);
    ofs << num_words << ",VectorTrie," << vec.query_time << "\n";
    ofs << num_words << ",ArrayTrie," << arr.query_time << "\n";
    ofs << num_words << ",HashTrie," << hash.query_time << "\n";
  }

  ofs = std::ofstream("plot_fill_factor_remove.csv");
  ofs << "num_words,variant,query_time_ns\n";
  for (const auto num_words : num_words_vec) {
    Instance instance = create_instance(num_words, min_word_length, max_word_length, 0, 0, 100000, chance_random_query);
    BenchmarkResult vec = run_benchmark_average<VectorTrie>(instance, "VectorTrie", runs);
    BenchmarkResult arr = run_benchmark_average<ArrayTrie>(instance, "ArrayTrie", runs);
    BenchmarkResult hash = run_benchmark_average<HashTrie>(instance, "HashTrie", runs);
    ofs << num_words << ",VectorTrie," << vec.query_time << "\n";
    ofs << num_words << ",ArrayTrie," << arr.query_time << "\n";
    ofs << num_words << ",HashTrie," << hash.query_time << "\n";
  }

  std::cout << "Plot data for Fill Factor written to plot_fill_factor.csv\n";
}

void
plot_word_length()
{
  std::ofstream ofs;

  ofs = std::ofstream("plot_word_length_construction_time.csv");
  ofs << "word_length,variant,construction_time_ns\n";
  for (int wl = 4; wl <= 32; wl += 4) {
    Instance instance = create_instance(200'000, wl, wl, 0, 0, 0, 0);
    BenchmarkResult vec = run_benchmark_average<VectorTrie>(instance, "VectorTrie", 5);
    BenchmarkResult arr = run_benchmark_average<ArrayTrie>(instance, "ArrayTrie", 5);
    BenchmarkResult hash = run_benchmark_average<HashTrie>(instance, "HashTrie", 5);
    ofs << wl << ",VectorTrie," << vec.construction_time << "\n";
    ofs << wl << ",ArrayTrie," << arr.construction_time << "\n";
    ofs << wl << ",HashTrie," << hash.construction_time << "\n";
  }

  ofs = std::ofstream("plot_word_length_construction_size.csv");
  ofs << "word_length,variant,construction_size\n";
  for (int wl = 4; wl <= 32; wl += 4) {
    Instance instance = create_instance(200'000, wl, wl, 0, 0, 0, 0);
    BenchmarkResult vec = run_benchmark_average<VectorTrie>(instance, "VectorTrie", 1);
    BenchmarkResult arr = run_benchmark_average<ArrayTrie>(instance, "ArrayTrie", 1);
    BenchmarkResult hash = run_benchmark_average<HashTrie>(instance, "HashTrie", 1);
    ofs << wl << ",VectorTrie," << vec.final_size << "\n";
    ofs << wl << ",ArrayTrie," << arr.final_size << "\n";
    ofs << wl << ",HashTrie," << hash.final_size << "\n";
  }

  ofs = std::ofstream("plot_word_length_insert_already_inserted.csv");
  ofs << "word_length,variant,query_time_ns\n";
  for (int wl = 4; wl <= 32; wl += 4) {
    Instance instance = create_instance(100000, wl, wl, 100000, 0, 0, 0);
    BenchmarkResult vec = run_benchmark_average<VectorTrie>(instance, "VectorTrie", 5);
    BenchmarkResult arr = run_benchmark_average<ArrayTrie>(instance, "ArrayTrie", 5);
    BenchmarkResult hash = run_benchmark_average<HashTrie>(instance, "HashTrie", 5);
    ofs << wl << ",VectorTrie," << vec.query_time << "\n";
    ofs << wl << ",ArrayTrie," << arr.query_time << "\n";
    ofs << wl << ",HashTrie," << hash.query_time << "\n";
  }

  ofs = std::ofstream("plot_word_length_insert_random.csv");
  ofs << "word_length,variant,query_time_ns\n";
  for (int wl = 4; wl <= 32; wl += 4) {
    Instance instance = create_instance(100000, wl, wl, 100000, 0, 0, 100);
    BenchmarkResult vec = run_benchmark_average<VectorTrie>(instance, "VectorTrie", 5);
    BenchmarkResult arr = run_benchmark_average<ArrayTrie>(instance, "ArrayTrie", 5);
    BenchmarkResult hash = run_benchmark_average<HashTrie>(instance, "HashTrie", 5);
    ofs << wl << ",VectorTrie," << vec.query_time << "\n";
    ofs << wl << ",ArrayTrie," << arr.query_time << "\n";
    ofs << wl << ",HashTrie," << hash.query_time << "\n";
  }

  ofs = std::ofstream("plot_word_length_contain_already_inserted.csv");
  ofs << "word_length,variant,query_time_ns\n";
  for (int wl = 4; wl <= 32; wl += 4) {
    Instance instance = create_instance(100000, wl, wl, 0, 100000, 0, 0);
    BenchmarkResult vec = run_benchmark_average<VectorTrie>(instance, "VectorTrie", 5);
    BenchmarkResult arr = run_benchmark_average<ArrayTrie>(instance, "ArrayTrie", 5);
    BenchmarkResult hash = run_benchmark_average<HashTrie>(instance, "HashTrie", 5);
    ofs << wl << ",VectorTrie," << vec.query_time << "\n";
    ofs << wl << ",ArrayTrie," << arr.query_time << "\n";
    ofs << wl << ",HashTrie," << hash.query_time << "\n";
  }

  ofs = std::ofstream("plot_word_length_contain_random.csv");
  ofs << "word_length,variant,query_time_ns\n";
  for (int wl = 4; wl <= 32; wl += 4) {
    Instance instance = create_instance(100000, wl, wl, 0, 100000, 0, 100);
    BenchmarkResult vec = run_benchmark_average<VectorTrie>(instance, "VectorTrie", 5);
    BenchmarkResult arr = run_benchmark_average<ArrayTrie>(instance, "ArrayTrie", 5);
    BenchmarkResult hash = run_benchmark_average<HashTrie>(instance, "HashTrie", 5);
    ofs << wl << ",VectorTrie," << vec.query_time << "\n";
    ofs << wl << ",ArrayTrie," << arr.query_time << "\n";
    ofs << wl << ",HashTrie," << hash.query_time << "\n";
  }

  ofs = std::ofstream("plot_word_length_remove_already_inserted.csv");
  ofs << "word_length,variant,query_time_ns\n";
  for (int wl = 4; wl <= 32; wl += 4) {
    Instance instance = create_instance(100000, wl, wl, 0, 0, 100000, 0);
    BenchmarkResult vec = run_benchmark_average<VectorTrie>(instance, "VectorTrie", 5);
    BenchmarkResult arr = run_benchmark_average<ArrayTrie>(instance, "ArrayTrie", 5);
    BenchmarkResult hash = run_benchmark_average<HashTrie>(instance, "HashTrie", 5);
    ofs << wl << ",VectorTrie," << vec.query_time << "\n";
    ofs << wl << ",ArrayTrie," << arr.query_time << "\n";
    ofs << wl << ",HashTrie," << hash.query_time << "\n";
  }

  ofs = std::ofstream("plot_word_length_remove_random.csv");
  ofs << "word_length,variant,query_time_ns\n";
  for (int wl = 4; wl <= 32; wl += 4) {
    Instance instance = create_instance(100000, wl, wl, 0, 0, 100000, 100);
    BenchmarkResult vec = run_benchmark_average<VectorTrie>(instance, "VectorTrie", 5);
    BenchmarkResult arr = run_benchmark_average<ArrayTrie>(instance, "ArrayTrie", 5);
    BenchmarkResult hash = run_benchmark_average<HashTrie>(instance, "HashTrie", 5);
    ofs << wl << ",VectorTrie," << vec.query_time << "\n";
    ofs << wl << ",ArrayTrie," << arr.query_time << "\n";
    ofs << wl << ",HashTrie," << hash.query_time << "\n";
  }

  std::cout << "Plot data for Word Length written\n";
}

void
plot_operation_mix()
{
  std::ofstream ofs;

  const auto num_words = 200'000;
  const auto total_queries = 300'000;
  const auto chance_random_query = 50;
  const auto min_word_length = 4, max_word_length = 24;
  const auto runs = 5;

  ofs = std::ofstream("plot_operation_mix.csv");
  ofs << "lookup_ratio,variant,query_time_ns\n";
  for (int ratio = 0; ratio <= 100; ratio += 5) {
    const auto num_lookup = total_queries * ratio / 100;
    const auto num_insert = (total_queries - num_lookup) / 2;
    const auto num_remove = (total_queries - num_lookup) / 2;
    Instance instance = create_instance(num_words, min_word_length, max_word_length, num_insert, num_lookup, num_remove, chance_random_query);
    BenchmarkResult vec = run_benchmark_average<VectorTrie>(instance, "VectorTrie", runs);
    BenchmarkResult arr = run_benchmark_average<ArrayTrie>(instance, "ArrayTrie", runs);
    BenchmarkResult hash = run_benchmark_average<HashTrie>(instance, "HashTrie", runs);
    ofs << ratio << ",VectorTrie," << vec.query_time << "\n";
    ofs << ratio << ",ArrayTrie," << arr.query_time << "\n";
    ofs << ratio << ",HashTrie," << hash.query_time << "\n";
  }

  std::cout << "Plot data for Operation Mix written to plot_operation_mix.csv\n";
}

int
main()
{
  std::cout << "Starting Trie Variant Plot Experiments...\n";

  plot_fill_factor();
  plot_word_length();
  plot_operation_mix();

  std::cout << "\nAll plot data files have been written to the working directory.\n";

  return 0;
}
