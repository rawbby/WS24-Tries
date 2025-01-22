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


// -----------------------------------------------------------------------------
// Helper to strip the trailing '$' from the input line, if it exists
// or any trailing 0-byte symbol representation. Adjust to your actual input needs.
// -----------------------------------------------------------------------------
std::string stripTerminator(const std::string &line) {
    // If the line ends with '$', remove it
    if (!line.empty() && line.back() == '$') {
        return line.substr(0, line.size() - 1);
    }
    // Otherwise, just return the line as is (or you could handle real '\0')
    // if the lines actually contain embedded '\0' characters in a different manner
    return line;
}

// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------
int main(int argc, char **argv) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0]
                  << " -variante=<1|2|3> <input_file> <query_file>\n";
        return 1;
    }

    // Parse arguments (simple approach)
    // Arg 1: something like "-variante=1"
    std::string varianteArg = argv[1];
    int variante = 0;
    if (varianteArg.rfind("-variante=", 0) == 0) {
        variante = std::stoi(varianteArg.substr(10)); // skip "-variante="
    } else {
        std::cerr << "Expected -variante=<1|2|3> as first argument.\n";
        return 1;
    }

    std::string inputFile = argv[2];
    std::string queryFile = argv[3];

    // Create the appropriate trie instance
    std::unique_ptr<TrieInterface> trie;
    std::string variantName;
    switch (variante) {
        case 1:
            trie = std::make_unique<TrieAdapter<VectorTrie>>();
            variantName = "vector_trie";
            break;
        case 2:
            trie = std::make_unique<TrieAdapter<ArrayTrie>>();
            variantName = "array_trie";
            break;
        case 3:
            trie = std::make_unique<TrieAdapter<HashTrie>>();
            variantName = "hash_trie";
            break;
        default:
            std::cerr << "Invalid variante: " << variante << std::endl;
            return 1;
    }

    // Read input file into memory
    std::vector<std::string> inputWords;
    {
        std::ifstream in(inputFile);
        if (!in) {
            std::cerr << "Error opening " << inputFile << std::endl;
            return 1;
        }
        std::string line;
        while (std::getline(in, line)) {
            // Remove trailing carriage return if needed (on Windows)
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            inputWords.push_back(stripTerminator(line));
        }
    }

    // Construct the trie and measure time
    auto startConstruction = std::chrono::high_resolution_clock::now();
    for (auto &w: inputWords) {
        trie->insert(w);
    }
    auto endConstruction = std::chrono::high_resolution_clock::now();
    auto constructionTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            endConstruction - startConstruction).count();

    // For memory-peak measurement, you can integrate platform-specific code
    // or call an external tool. Here we skip the actual implementation:
    double memoryPeakMiB = 0.0; // placeholder

    // Read queries, store them, measure time for queries
    std::vector<std::pair<std::string, char>> queries;
    {
        std::ifstream qf(queryFile);
        if (!qf) {
            std::cerr << "Error opening " << queryFile << std::endl;
            return 1;
        }
        std::string line;
        while (std::getline(qf, line)) {
            // Each line is "word$ <op>"
            // op is 'c' (contains), 'd' (delete), 'i' (insert)
            // Example: "car$ c"
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            // Split line into two parts: word and operation
            // For simplicity, assume there's exactly one space before the operation.
            // Adjust parsing as needed for your data format.
            auto spacePos = line.find_last_of(' ');
            if (spacePos == std::string::npos) {
                continue; // skip invalid line
            }
            std::string wordPart = line.substr(0, spacePos);
            char op = line.substr(spacePos + 1)[0];

            // Strip terminator from wordPart
            wordPart = stripTerminator(wordPart);
            queries.emplace_back(wordPart, op);
        }
    }

    // Result file
    std::string resultFilename = "result_" + inputFile + ".txt";
    std::ofstream resultOut(resultFilename);
    if (!resultOut) {
        std::cerr << "Error opening " << resultFilename << std::endl;
        return 1;
    }

    auto startQuery = std::chrono::high_resolution_clock::now();
    for (auto &q: queries) {
        const std::string &qw = q.first;
        char op = q.second;
        bool res = false;
        switch (op) {
            case 'c':
                res = trie->contains(qw);
                break;
            case 'i':
                res = trie->insert(qw);
                break;
            case 'd':
                res = trie->remove(qw);
                break;
            default:
                break;
        }
        resultOut << (res ? "true\n" : "false\n");
    }
    auto endQuery = std::chrono::high_resolution_clock::now();
    auto queryTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            endQuery - startQuery).count();

    // Print required result line to stdout
    std::string yourName = "YourName"; // replace with your actual name
    std::cout << "RESULT name=" << yourName
              << " trie variant=" << variantName
              << " trie construction time=" << constructionTimeMs
              << " trie construction memory=" << memoryPeakMiB
              << " query time=" << queryTimeMs
              << std::endl;

    return 0;
}
