#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

// Include your trie implementations.
#include <array_trie.hpp>
#include <hash_trie.hpp>
#include <vector_trie.hpp>

#define MIN_WORD_LENGTH 1
#define MAX_WORD_LENGTH 32

// -----------------------------------------------------------------------------
// Helper to prevent unwanted compiler optimizations.
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

// -----------------------------------------------------------------------------
// Random word generator.
static std::string
random_word(std::mt19937& rng, int min_word_length, int max_word_length)
{
  static constexpr char chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  std::uniform_int_distribution<int> length_dist(min_word_length, max_word_length);
  std::uniform_int_distribution<int> chars_dist(0, sizeof(chars) - 2);
  int length = length_dist(rng);
  std::string result;
  result.reserve(length + 1);
  for (int i = 0; i < length; ++i)
    result.push_back(chars[chars_dist(rng)]);
  result.push_back('$');
  return result;
}

// -----------------------------------------------------------------------------
// Instance holds the words and queries used for a benchmark run.
struct Instance
{
  int num_words;
  int min_word_length;
  int max_word_length;
  int num_insert_queries;
  int num_contains_queries;
  int num_remove_queries;
  int chance_random_query;
  std::vector<std::string> words;
  std::vector<std::pair<int, std::string>> queries;
};

// Creates an instance given the parameters.
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

  instance.words.reserve(num_words);
  for (int i = 0; i < num_words; ++i)
    instance.words.push_back(random_word(rng, min_word_length, max_word_length));

  std::uniform_int_distribution<int> percent_dist(0, 99);
  std::uniform_int_distribution<int> index_dist(0, num_words - 1);
  instance.queries.reserve(num_insert_queries + num_remove_queries + num_contains_queries);

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

// -----------------------------------------------------------------------------
// Structure to hold benchmark results.
struct BenchmarkResult
{
  std::string variant;
  long construction_time; // in nanoseconds
  long query_time;        // in nanoseconds
  long final_size;
};

// Run one benchmark instance for a given trie variant.
template<typename Trie>
BenchmarkResult
run_benchmark_instance(const Instance& instance, const std::string& variant_name)
{
  Trie trie;

  // --- Construction Phase ---
  auto start_construction = std::chrono::steady_clock::now();
  for (const auto& word : instance.words)
    trie.insert(word);
  auto end_construction = std::chrono::steady_clock::now();
  long construction_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_construction - start_construction).count();
  long final_size = trie.size();

  // --- Query Phase ---
  volatile int dummy_accum = 0;
  auto start_query = std::chrono::steady_clock::now();
  for (const auto& query : instance.queries) {
    int op = query.first;
    const std::string& word = query.second;
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
  auto end_query = std::chrono::steady_clock::now();
  long query_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_query - start_query).count();

  return { variant_name, construction_time, query_time, final_size };
}

// Run several runs and compute average results for one trie variant.
template<typename Trie>
BenchmarkResult
run_benchmark_average(const Instance& instance, const std::string& variant_name, int runs)
{
  long total_construction = 0;
  long total_query = 0;
  long total_size = 0;
  for (int i = 0; i < runs; ++i) {
    BenchmarkResult res = run_benchmark_instance<Trie>(instance, variant_name);
    total_construction += res.construction_time;
    total_query += res.query_time;
    total_size += res.final_size;
  }
  return { variant_name, total_construction / runs, total_query / runs, total_size / runs };
}

// -----------------------------------------------------------------------------
// Experiment 1: Varying Fill Factor (by varying the number of words)
// New range: from 10,000 to 200,000 words.
void
experiment_fill_factor()
{
  std::ofstream ofs("experiment_fill_factor_results.txt");
  ofs << "num_words,Variant,ConstructionTime(ns),QueryTime(ns),FinalSize\n";
  std::cout << "\n--- Fill Factor Experiment ---\n";

  std::vector<int> num_words_vec = { 10000, 50000, 100000, 200000 };
  const int min_word_length = 8, max_word_length = 16;
  // Increase queries to add more work.
  const int num_insert_queries = 50000;
  const int num_contains_queries = 50000;
  const int num_remove_queries = 50000;
  const int chance_random_query = 50;
  int runs = 3;

  for (int num_words : num_words_vec) {
    std::cout << "\nTesting with num_words = " << num_words << std::endl;
    Instance instance =
      create_instance(num_words, min_word_length, max_word_length, num_insert_queries, num_contains_queries, num_remove_queries, chance_random_query);

    BenchmarkResult vec_avg = run_benchmark_average<VectorTrie>(instance, "VectorTrie", runs);
    BenchmarkResult arr_avg = run_benchmark_average<ArrayTrie>(instance, "ArrayTrie", runs);
    BenchmarkResult hash_avg = run_benchmark_average<HashTrie>(instance, "HashTrie", runs);

    ofs << num_words << "," << vec_avg.variant << "," << vec_avg.construction_time << "," << vec_avg.query_time << "," << vec_avg.final_size << "\n";
    ofs << num_words << "," << arr_avg.variant << "," << arr_avg.construction_time << "," << arr_avg.query_time << "," << arr_avg.final_size << "\n";
    ofs << num_words << "," << hash_avg.variant << "," << hash_avg.construction_time << "," << hash_avg.query_time << "," << hash_avg.final_size << "\n";

    std::cout << "num_words: " << num_words << "\n"
              << "  VectorTrie: Construction=" << vec_avg.construction_time << " ns, Query=" << vec_avg.query_time << " ns, Size=" << vec_avg.final_size << "\n"
              << "  ArrayTrie:  Construction=" << arr_avg.construction_time << " ns, Query=" << arr_avg.query_time << " ns, Size=" << arr_avg.final_size << "\n"
              << "  HashTrie:   Construction=" << hash_avg.construction_time << " ns, Query=" << hash_avg.query_time << " ns, Size=" << hash_avg.final_size
              << "\n";
  }

  std::cout << "\nFill Factor Reasoning:\n"
            << "  Increasing the number of words should increase both construction and query times.\n"
            << "  A trie implementation that scales gracefully will show a less steep increase in latency.\n";
}

// -----------------------------------------------------------------------------
// Experiment 2: Varying Word Length (short, medium, and long words)
// Increase instance size and query counts to see millisecond-level timings.
void
experiment_word_length()
{
  std::ofstream ofs("experiment_word_length_results.txt");
  ofs << "WordRange,Variant,ConstructionTime(ns),QueryTime(ns),FinalSize\n";
  std::cout << "\n--- Word Length Experiment ---\n";

  std::vector<std::pair<int, int>> word_ranges = { { 3, 5 }, { 8, 16 }, { 20, 30 } };
  const int num_words = 50000;
  const int num_insert_queries = 50000;
  const int num_contains_queries = 50000;
  const int num_remove_queries = 50000;
  const int chance_random_query = 50;
  int runs = 3;

  for (auto range : word_ranges) {
    int min_word = range.first;
    int max_word = range.second;
    std::ostringstream range_str;
    range_str << min_word << "-" << max_word;
    std::cout << "\nTesting with word length range: " << range_str.str() << std::endl;

    Instance instance = create_instance(num_words, min_word, max_word, num_insert_queries, num_contains_queries, num_remove_queries, chance_random_query);

    BenchmarkResult vec_avg = run_benchmark_average<VectorTrie>(instance, "VectorTrie", runs);
    BenchmarkResult arr_avg = run_benchmark_average<ArrayTrie>(instance, "ArrayTrie", runs);
    BenchmarkResult hash_avg = run_benchmark_average<HashTrie>(instance, "HashTrie", runs);

    ofs << range_str.str() << "," << vec_avg.variant << "," << vec_avg.construction_time << "," << vec_avg.query_time << "," << vec_avg.final_size << "\n";
    ofs << range_str.str() << "," << arr_avg.variant << "," << arr_avg.construction_time << "," << arr_avg.query_time << "," << arr_avg.final_size << "\n";
    ofs << range_str.str() << "," << hash_avg.variant << "," << hash_avg.construction_time << "," << hash_avg.query_time << "," << hash_avg.final_size << "\n";

    std::cout << "Word Range: " << range_str.str() << "\n"
              << "  VectorTrie: Construction=" << vec_avg.construction_time << " ns, Query=" << vec_avg.query_time << " ns, Size=" << vec_avg.final_size << "\n"
              << "  ArrayTrie:  Construction=" << arr_avg.construction_time << " ns, Query=" << arr_avg.query_time << " ns, Size=" << arr_avg.final_size << "\n"
              << "  HashTrie:   Construction=" << hash_avg.construction_time << " ns, Query=" << hash_avg.query_time << " ns, Size=" << hash_avg.final_size
              << "\n";
  }

  std::cout << "\nWord Length Reasoning:\n"
            << "  Shorter words lead to shallower tries which may yield faster lookups,\n"
            << "  but may also increase branch collisions. Longer words create deeper structures,\n"
            << "  potentially affecting both insertion and query performance.\n";
}

// -----------------------------------------------------------------------------
// Experiment 3: Varying Operation Mix (balanced, insert heavy, lookup heavy)
// Increase instance size and query counts.
void
experiment_operation_mix()
{
  std::ofstream ofs("experiment_operation_mix_results.txt");
  ofs << "MixType,Variant,ConstructionTime(ns),QueryTime(ns),FinalSize\n";
  std::cout << "\n--- Operation Mix Experiment ---\n";

  struct OpMix
  {
    int inserts;
    int removes;
    int contains;
    std::string description;
  };
  std::vector<OpMix> mixes = { { 50000, 50000, 50000, "Balanced" }, { 50000, 5000, 5000, "Insert Heavy" }, { 5000, 5000, 50000, "Lookup Heavy" } };
  const int num_words = 50000;
  const int min_word_length = 8, max_word_length = 16;
  const int chance_random_query = 50;
  int runs = 3;

  for (auto mix : mixes) {
    std::cout << "\nTesting Operation Mix: " << mix.description << " (Inserts: " << mix.inserts << ", Removes: " << mix.removes
              << ", Contains: " << mix.contains << ")\n";

    Instance instance = create_instance(num_words, min_word_length, max_word_length, mix.inserts, mix.contains, mix.removes, chance_random_query);

    BenchmarkResult vec_avg = run_benchmark_average<VectorTrie>(instance, "VectorTrie", runs);
    BenchmarkResult arr_avg = run_benchmark_average<ArrayTrie>(instance, "ArrayTrie", runs);
    BenchmarkResult hash_avg = run_benchmark_average<HashTrie>(instance, "HashTrie", runs);

    ofs << mix.description << "," << vec_avg.variant << "," << vec_avg.construction_time << "," << vec_avg.query_time << "," << vec_avg.final_size << "\n";
    ofs << mix.description << "," << arr_avg.variant << "," << arr_avg.construction_time << "," << arr_avg.query_time << "," << arr_avg.final_size << "\n";
    ofs << mix.description << "," << hash_avg.variant << "," << hash_avg.construction_time << "," << hash_avg.query_time << "," << hash_avg.final_size << "\n";

    std::cout << "Operation Mix: " << mix.description << "\n"
              << "  VectorTrie: Construction=" << vec_avg.construction_time << " ns, Query=" << vec_avg.query_time << " ns, Size=" << vec_avg.final_size << "\n"
              << "  ArrayTrie:  Construction=" << arr_avg.construction_time << " ns, Query=" << arr_avg.query_time << " ns, Size=" << arr_avg.final_size << "\n"
              << "  HashTrie:   Construction=" << hash_avg.construction_time << " ns, Query=" << hash_avg.query_time << " ns, Size=" << hash_avg.final_size
              << "\n";
  }

  std::cout << "\nOperation Mix Reasoning:\n"
            << "  The relative frequency of insert, remove, and lookup operations stresses different parts\n"
            << "  of the implementations. For example, lookup heavy scenarios emphasize search efficiency,\n"
            << "  while insert heavy scenarios test allocation and restructuring overhead.\n";
}

// -----------------------------------------------------------------------------
// Experiment 4: Varying Instance Size (small, medium, large)
// New sizes: from 10,000 to 1,000,000 words.
void
experiment_instance_size()
{
  std::ofstream ofs("experiment_instance_size_results.txt");
  ofs << "InstanceSize,Variant,ConstructionTime(ns),QueryTime(ns),FinalSize\n";
  std::cout << "\n--- Instance Size Experiment ---\n";

  struct InstanceSize
  {
    int num_words;
    int num_insert_queries;
    int num_contains_queries;
    int num_remove_queries;
    std::string description;
  };
  std::vector<InstanceSize> sizes = { { 10000, 50000, 50000, 50000, "Small" },        { 50000, 50000, 50000, 50000, "Medium" },
                                      { 100000, 50000, 50000, 50000, "Large" },       { 200000, 50000, 50000, 50000, "Extra Large" },
                                      { 500000, 50000, 50000, 50000, "Ultra Large" }, { 1000000, 50000, 50000, 50000, "Mega Large" } };
  const int min_word_length = 8, max_word_length = 16;
  const int chance_random_query = 50;
  int runs = 3;

  for (auto size : sizes) {
    std::cout << "\nTesting Instance Size: " << size.description << " (num_words: " << size.num_words << ")\n";
    Instance instance = create_instance(
      size.num_words, min_word_length, max_word_length, size.num_insert_queries, size.num_contains_queries, size.num_remove_queries, chance_random_query);

    BenchmarkResult vec_avg = run_benchmark_average<VectorTrie>(instance, "VectorTrie", runs);
    BenchmarkResult arr_avg = run_benchmark_average<ArrayTrie>(instance, "ArrayTrie", runs);
    BenchmarkResult hash_avg = run_benchmark_average<HashTrie>(instance, "HashTrie", runs);

    ofs << size.description << "," << vec_avg.variant << "," << vec_avg.construction_time << "," << vec_avg.query_time << "," << vec_avg.final_size << "\n";
    ofs << size.description << "," << arr_avg.variant << "," << arr_avg.construction_time << "," << arr_avg.query_time << "," << arr_avg.final_size << "\n";
    ofs << size.description << "," << hash_avg.variant << "," << hash_avg.construction_time << "," << hash_avg.query_time << "," << hash_avg.final_size << "\n";

    std::cout << "Instance Size: " << size.description << "\n"
              << "  VectorTrie: Construction=" << vec_avg.construction_time << " ns, Query=" << vec_avg.query_time << " ns, Size=" << vec_avg.final_size << "\n"
              << "  ArrayTrie:  Construction=" << arr_avg.construction_time << " ns, Query=" << arr_avg.query_time << " ns, Size=" << arr_avg.final_size << "\n"
              << "  HashTrie:   Construction=" << hash_avg.construction_time << " ns, Query=" << hash_avg.query_time << " ns, Size=" << hash_avg.final_size
              << "\n";
  }

  std::cout << "\nInstance Size Reasoning:\n"
            << "  By increasing the instance size, we can observe how each trie scales in terms of both\n"
            << "  time (construction and query) and memory (final size).\n";
}

// -----------------------------------------------------------------------------
// Experiment 5: Operation Isolation Experiment (only one query type per run)
// Increase query counts from 10,000 to 100,000 in steps of 10,000.
void
experiment_operation_isolation()
{
  std::ofstream ofs("experiment_operation_isolation_results.txt");
  ofs << "OperationType,Variant,ConstructionTime(ns),QueryTime(ns),FinalSize\n";
  std::cout << "\n--- Operation Isolation Experiment ---\n";

  const int num_words = 50000;
  const int min_word_length = 8, max_word_length = 16;
  const int chance_random_query = 50;
  int runs = 3;

  struct OpIsol
  {
    int inserts;
    int removes;
    int contains;
    std::string description;
  };
  std::vector<OpIsol> ops = { { 100000, 0, 0, "Insert Only" }, { 0, 100000, 0, "Remove Only" }, { 0, 0, 100000, "Lookup Only" } };

  for (auto op : ops) {
    std::cout << "\nTesting Operation Isolation: " << op.description << "\n";
    Instance instance = create_instance(num_words, min_word_length, max_word_length, op.inserts, op.contains, op.removes, chance_random_query);

    BenchmarkResult vec_avg = run_benchmark_average<VectorTrie>(instance, "VectorTrie", runs);
    BenchmarkResult arr_avg = run_benchmark_average<ArrayTrie>(instance, "ArrayTrie", runs);
    BenchmarkResult hash_avg = run_benchmark_average<HashTrie>(instance, "HashTrie", runs);

    ofs << op.description << "," << vec_avg.variant << "," << vec_avg.construction_time << "," << vec_avg.query_time << "," << vec_avg.final_size << "\n";
    ofs << op.description << "," << arr_avg.variant << "," << arr_avg.construction_time << "," << arr_avg.query_time << "," << arr_avg.final_size << "\n";
    ofs << op.description << "," << hash_avg.variant << "," << hash_avg.construction_time << "," << hash_avg.query_time << "," << hash_avg.final_size << "\n";

    std::cout << op.description << ":\n"
              << "  VectorTrie: Construction=" << vec_avg.construction_time << " ns, Query=" << vec_avg.query_time << " ns, Size=" << vec_avg.final_size << "\n"
              << "  ArrayTrie:  Construction=" << arr_avg.construction_time << " ns, Query=" << arr_avg.query_time << " ns, Size=" << arr_avg.final_size << "\n"
              << "  HashTrie:   Construction=" << hash_avg.construction_time << " ns, Query=" << hash_avg.query_time << " ns, Size=" << hash_avg.final_size
              << "\n";
  }

  std::cout << "\nOperation Isolation Reasoning:\n"
            << "  By isolating each operation (insert, remove, lookup) we can better understand\n"
            << "  which operations are the performance bottlenecks for each trie variant.\n";
}

// -----------------------------------------------------------------------------
// Experiment 6: Large Instance Experiment (very large datasets)
// Increase to 200,000, 500,000, and 1,000,000 words.
void
experiment_large_instance()
{
  std::ofstream ofs("experiment_large_instance_results.txt");
  ofs << "InstanceSize,Variant,ConstructionTime(ns),QueryTime(ns),FinalSize\n";
  std::cout << "\n--- Large Instance Experiment ---\n";

  struct LargeInstance
  {
    int num_words;
    int num_insert_queries;
    int num_contains_queries;
    int num_remove_queries;
    std::string description;
  };
  std::vector<LargeInstance> sizes = { { 200000, 50000, 50000, 50000, "Extra Large" },
                                       { 500000, 50000, 50000, 50000, "Ultra Large" },
                                       { 1000000, 50000, 50000, 50000, "Mega Large" } };
  const int min_word_length = 8, max_word_length = 16;
  const int chance_random_query = 50;
  int runs = 3;

  for (auto size : sizes) {
    std::cout << "\nTesting Large Instance: " << size.description << " (num_words: " << size.num_words << ")\n";
    Instance instance = create_instance(
      size.num_words, min_word_length, max_word_length, size.num_insert_queries, size.num_contains_queries, size.num_remove_queries, chance_random_query);

    BenchmarkResult vec_avg = run_benchmark_average<VectorTrie>(instance, "VectorTrie", runs);
    BenchmarkResult arr_avg = run_benchmark_average<ArrayTrie>(instance, "ArrayTrie", runs);
    BenchmarkResult hash_avg = run_benchmark_average<HashTrie>(instance, "HashTrie", runs);

    ofs << size.description << "," << vec_avg.variant << "," << vec_avg.construction_time << "," << vec_avg.query_time << "," << vec_avg.final_size << "\n";
    ofs << size.description << "," << arr_avg.variant << "," << arr_avg.construction_time << "," << arr_avg.query_time << "," << arr_avg.final_size << "\n";
    ofs << size.description << "," << hash_avg.variant << "," << hash_avg.construction_time << "," << hash_avg.query_time << "," << hash_avg.final_size << "\n";

    std::cout << size.description << ":\n"
              << "  VectorTrie: Construction=" << vec_avg.construction_time << " ns, Query=" << vec_avg.query_time << " ns, Size=" << vec_avg.final_size << "\n"
              << "  ArrayTrie:  Construction=" << arr_avg.construction_time << " ns, Query=" << arr_avg.query_time << " ns, Size=" << arr_avg.final_size << "\n"
              << "  HashTrie:   Construction=" << hash_avg.construction_time << " ns, Query=" << hash_avg.query_time << " ns, Size=" << hash_avg.final_size
              << "\n";
  }

  std::cout << "\nLarge Instance Reasoning:\n"
            << "  Larger datasets can expose asymptotic scaling behaviors, memory allocation overheads,\n"
            << "  and cache effects that are not visible with smaller instances.\n";
}

// -----------------------------------------------------------------------------
// Plot 1: Fill Factor Experiment
// Vary the number of words from 10,000 to 200,000 and record construction time,
// query time, and final size.
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

// -----------------------------------------------------------------------------
// Plot 2: Word Length Experiment
// Vary the fixed word length from 3 to 30 (using min_word_length == max_word_length)
// and record the performance.
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

  // ofs = std::ofstream("plot_word_length_insert_already_inserted.csv");
  // ofs << "word_length,variant,query_time_ns\n";
  // for (int wl = 4; wl <= 32; wl += 4) {
  //   Instance instance = create_instance(100000, wl, wl, 100000, 0, 0, 0);
  //   BenchmarkResult vec = run_benchmark_average<VectorTrie>(instance, "VectorTrie", 5);
  //   BenchmarkResult arr = run_benchmark_average<ArrayTrie>(instance, "ArrayTrie", 5);
  //   BenchmarkResult hash = run_benchmark_average<HashTrie>(instance, "HashTrie", 5);
  //   ofs << wl << ",VectorTrie," << vec.query_time << "\n";
  //   ofs << wl << ",ArrayTrie," << arr.query_time << "\n";
  //   ofs << wl << ",HashTrie," << hash.query_time << "\n";
  // }

  // ofs = std::ofstream("plot_word_length_insert_random.csv");
  // ofs << "word_length,variant,query_time_ns\n";
  // for (int wl = 4; wl <= 32; wl += 4) {
  //   Instance instance = create_instance(100000, wl, wl, 100000, 0, 0, 100);
  //   BenchmarkResult vec = run_benchmark_average<VectorTrie>(instance, "VectorTrie", 5);
  //   BenchmarkResult arr = run_benchmark_average<ArrayTrie>(instance, "ArrayTrie", 5);
  //   BenchmarkResult hash = run_benchmark_average<HashTrie>(instance, "HashTrie", 5);
  //   ofs << wl << ",VectorTrie," << vec.query_time << "\n";
  //   ofs << wl << ",ArrayTrie," << arr.query_time << "\n";
  //   ofs << wl << ",HashTrie," << hash.query_time << "\n";
  // }

  // ofs = std::ofstream("plot_word_length_contain_already_inserted.csv");
  // ofs << "word_length,variant,query_time_ns\n";
  // for (int wl = 4; wl <= 32; wl += 4) {
  //   Instance instance = create_instance(100000, wl, wl, 0, 100000, 0, 0);
  //   BenchmarkResult vec = run_benchmark_average<VectorTrie>(instance, "VectorTrie", 5);
  //   BenchmarkResult arr = run_benchmark_average<ArrayTrie>(instance, "ArrayTrie", 5);
  //   BenchmarkResult hash = run_benchmark_average<HashTrie>(instance, "HashTrie", 5);
  //   ofs << wl << ",VectorTrie," << vec.query_time << "\n";
  //   ofs << wl << ",ArrayTrie," << arr.query_time << "\n";
  //   ofs << wl << ",HashTrie," << hash.query_time << "\n";
  // }

  // ofs = std::ofstream("plot_word_length_contain_random.csv");
  // ofs << "word_length,variant,query_time_ns\n";
  // for (int wl = 4; wl <= 32; wl += 4) {
  //   Instance instance = create_instance(100000, wl, wl, 0, 100000, 0, 100);
  //   BenchmarkResult vec = run_benchmark_average<VectorTrie>(instance, "VectorTrie", 5);
  //   BenchmarkResult arr = run_benchmark_average<ArrayTrie>(instance, "ArrayTrie", 5);
  //   BenchmarkResult hash = run_benchmark_average<HashTrie>(instance, "HashTrie", 5);
  //   ofs << wl << ",VectorTrie," << vec.query_time << "\n";
  //   ofs << wl << ",ArrayTrie," << arr.query_time << "\n";
  //   ofs << wl << ",HashTrie," << hash.query_time << "\n";
  // }

  // ofs = std::ofstream("plot_word_length_remove_already_inserted.csv");
  // ofs << "word_length,variant,query_time_ns\n";
  // for (int wl = 4; wl <= 32; wl += 4) {
  //   Instance instance = create_instance(100000, wl, wl, 0, 0, 100000, 0);
  //   BenchmarkResult vec = run_benchmark_average<VectorTrie>(instance, "VectorTrie", 5);
  //   BenchmarkResult arr = run_benchmark_average<ArrayTrie>(instance, "ArrayTrie", 5);
  //   BenchmarkResult hash = run_benchmark_average<HashTrie>(instance, "HashTrie", 5);
  //   ofs << wl << ",VectorTrie," << vec.query_time << "\n";
  //   ofs << wl << ",ArrayTrie," << arr.query_time << "\n";
  //   ofs << wl << ",HashTrie," << hash.query_time << "\n";
  // }

  // ofs = std::ofstream("plot_word_length_remove_random.csv");
  // ofs << "word_length,variant,query_time_ns\n";
  // for (int wl = 4; wl <= 32; wl += 4) {
  //   Instance instance = create_instance(100000, wl, wl, 0, 0, 100000, 100);
  //   BenchmarkResult vec = run_benchmark_average<VectorTrie>(instance, "VectorTrie", 5);
  //   BenchmarkResult arr = run_benchmark_average<ArrayTrie>(instance, "ArrayTrie", 5);
  //   BenchmarkResult hash = run_benchmark_average<HashTrie>(instance, "HashTrie", 5);
  //   ofs << wl << ",VectorTrie," << vec.query_time << "\n";
  //   ofs << wl << ",ArrayTrie," << arr.query_time << "\n";
  //   ofs << wl << ",HashTrie," << hash.query_time << "\n";
  // }

  std::cout << "Plot data for Word Length written\n";
}

// -----------------------------------------------------------------------------
// Plot 3: Operation Mix Experiment
// Vary the ratio of lookup operations from 0% to 100% (with no removals) while keeping
// the total number of queries fixed.
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

// -----------------------------------------------------------------------------
// Plot 4: Instance Size Experiment
// Vary instance size (number of words) over a broad range.
void
plot_instance_size()
{
  std::ofstream ofs("plot_instance_size.csv");
  ofs << "num_words,variant,construction_time_ns,query_time_ns,final_size\n";

  std::vector sizes = { 25000, 50000, 100000, 250000, 500000, 1000000, 2500000 };
  const int min_word_length = 1, max_word_length = 12;
  const int num_insert_queries = 50000;
  const int num_contains_queries = 50000;
  const int num_remove_queries = 50000;
  const int chance_random_query = 50;
  int runs = 3;

  for (int num_words : sizes) {
    Instance instance =
      create_instance(num_words, min_word_length, max_word_length, num_insert_queries, num_contains_queries, num_remove_queries, chance_random_query);
    BenchmarkResult vec = run_benchmark_average<VectorTrie>(instance, "VectorTrie", runs);
    BenchmarkResult arr = run_benchmark_average<ArrayTrie>(instance, "ArrayTrie", runs);
    BenchmarkResult hash = run_benchmark_average<HashTrie>(instance, "HashTrie", runs);

    ofs << num_words << ",VectorTrie," << vec.construction_time << "," << vec.query_time << "," << vec.final_size << "\n";
    ofs << num_words << ",ArrayTrie," << arr.construction_time << "," << arr.query_time << "," << arr.final_size << "\n";
    ofs << num_words << ",HashTrie," << hash.construction_time << "," << hash.query_time << "," << hash.final_size << "\n";
  }
  std::cout << "Plot data for Instance Size written to plot_instance_size.csv\n";
}

// -----------------------------------------------------------------------------
// Plot 5: Operation Isolation Experiment
// For each operation type (insert, remove, lookup), vary the number of queries
// from 10,000 to 100,000 in steps of 10,000.
void
plot_operation_isolation()
{
  std::ofstream ofs("plot_operation_isolation.csv");
  ofs << "operation_type,query_count,variant,construction_time_ns,query_time_ns,final_size\n";

  const int num_words = 250000;
  const int min_word_length = 1, max_word_length = 12;
  const int chance_random_query = 50;
  int runs = 3;

  std::vector<std::string> operations = { "Insert", "Remove", "Lookup" };
  for (const auto& op_type : operations) {
    for (int q = 10000; q <= 100000; q += 10000) {
      int ins = 0, rem = 0, look = 0;
      if (op_type == "Insert")
        ins = q;
      else if (op_type == "Remove")
        rem = q;
      else if (op_type == "Lookup")
        look = q;
      Instance instance = create_instance(num_words, min_word_length, max_word_length, ins, look, rem, chance_random_query);
      BenchmarkResult vec = run_benchmark_average<VectorTrie>(instance, "VectorTrie", runs);
      BenchmarkResult arr = run_benchmark_average<ArrayTrie>(instance, "ArrayTrie", runs);
      BenchmarkResult hash = run_benchmark_average<HashTrie>(instance, "HashTrie", runs);

      ofs << op_type << "," << q << ",VectorTrie," << vec.construction_time << "," << vec.query_time << "," << vec.final_size << "\n";
      ofs << op_type << "," << q << ",ArrayTrie," << arr.construction_time << "," << arr.query_time << "," << arr.final_size << "\n";
      ofs << op_type << "," << q << ",HashTrie," << hash.construction_time << "," << hash.query_time << "," << hash.final_size << "\n";
    }
  }
  std::cout << "Plot data for Operation Isolation written to plot_operation_isolation.csv\n";
}

// -----------------------------------------------------------------------------
// Main function: run all experiments and then generate plot data.
int
main()
{
  // std::cout << "Starting Trie Variant Experiments with increased instance sizes...\n";
  // experiment_fill_factor();
  // experiment_word_length();
  // experiment_operation_mix();
  // experiment_instance_size();
  // experiment_operation_isolation();
  // experiment_large_instance();
  // std::cout << "\nExperiments completed. Results are stored in the working directory.\n";

  std::cout << "Starting Trie Variant Plot Experiments...\n";

  // plot_fill_factor();
  plot_word_length();
  // plot_operation_mix();
  // plot_instance_size();
  // plot_operation_isolation();

  std::cout << "\nAll plot data files have been written to the working directory.\n";

  return 0;
}
