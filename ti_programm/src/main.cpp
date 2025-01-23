#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <memory>
#include <vector>

#include <trie_adapter.hpp>
#include <array_trie.hpp>
#include <vector_trie.hpp>
#include <hash_trie.hpp>

inline auto timestamp() {
    return std::chrono::high_resolution_clock::now();
}

auto millis(auto time_difference) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(time_difference).count();
}

int main(int argc, char **argv) {
    if (argc != 4) {
        std::cerr << "Usage: ti_programm -variant_value=<1|2|3> <eingabe_datei> <query_datei>" << std::endl;
        std::exit(1);
    }

    const auto variant_param = std::string{argv[1]};
    const auto variant_value = variant_param.back() - '0';
    const auto input_file = std::string{argv[2]};
    const auto query_file = std::string{argv[3]};

    std::unique_ptr<TrieInterface> trie;
    std::string variant_name;
    switch (variant_value) {
        case 1:
            trie = std::make_unique<TrieAdapter<VectorTrie> >();
            variant_name = "vector_trie";
            break;
        case 2:
            trie = std::make_unique<TrieAdapter<ArrayTrie> >();
            variant_name = "array_trie";
            break;
        case 3:
            trie = std::make_unique<TrieAdapter<HashTrie> >();
            variant_name = "hash_trie";
            break;
        default:
            std::cerr << "Invalid variant: " << variant_value << std::endl;
            std::exit(1);
    }

    auto input_words = std::vector<std::string>{};
    auto input_stream = std::ifstream{input_file};

    if (!input_stream) {
        std::cerr << "Error opening " << input_file << std::endl;
        std::exit(1);
    }

    std::string line;
    while (std::getline(input_stream, line)) {
        while (!line.empty() && !std::isalnum(line.back()))
            line.pop_back();
        if (line.empty())
            continue;
        input_words.push_back(line);
    }

    const auto start_construction = timestamp();
    for (auto &w: input_words) {
        if (!trie->insert(w)) {
            std::cerr << "Error inserting " << w << std::endl;
            std::exit(1);
        }
    }
    const auto end_construction = timestamp();
    const auto time_construction_ms = millis(end_construction - start_construction);

    double memoryPeakMiB = 0.0; // placeholder

    auto queries = std::vector<std::pair<std::string, char> >{};
    auto query_stream = std::ifstream{query_file};

    if (!query_stream) {
        std::cerr << "Error opening " << query_file << std::endl;
        std::exit(1);
    }

    char operation;
    while (std::getline(query_stream, line)) {
        while (!line.empty() && !std::isalnum(line.back()))
            line.pop_back();
        if (line.empty())
            continue;

        operation = line.back();
        line.pop_back();

        while (!line.empty() && !std::isalnum(line.back()))
            line.pop_back();
        if (line.empty())
            continue;

        queries.emplace_back(line, operation);
    }

    const auto result_filename = std::string{"result_" + input_file + ".txt"};
    auto result_stream = std::ofstream{result_filename};

    if (!result_stream) {
        std::cerr << "Error opening " << result_filename << std::endl;
        std::exit(1);
    }

    const auto start_queries = timestamp();
    for (const auto &query: queries) {
        bool res = false;
        switch (query.second) {
            case 'c':
                res = trie->contains(query.first);
                break;
            case 'i':
                res = trie->insert(query.first);
                break;
            case 'd':
                res = trie->remove(query.first);
                break;
            default:
                break;
        }
        result_stream << (res ? "true\n" : "false\n");
    }
    const auto end_queries = timestamp();
    const auto time_queries_ms = millis(end_queries - start_queries);

    std::cout << "RESULT name=Robert trie_variant=" << variant_name
            << " trie_construction_time=" << time_construction_ms
            << " trie_construction_memory=" << memoryPeakMiB
            << " query_time=" << time_queries_ms << std::endl;
}
