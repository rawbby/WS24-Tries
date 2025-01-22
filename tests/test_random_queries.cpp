#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <algorithm>

#include <array_trie.hpp>
#include <vector_trie.hpp>
#include <hash_trie.hpp>

#include "test_util.hpp"

#define MIN_WORD_LENGTH 1
#define MAX_WORD_LENGTH 32
#define NUM_WORDS 5'000
#define NUM_QUERIES 500'000
#define CHANCE_RANDOM_QUERY 10

static std::string random_word(std::mt19937 &rng) {
    static constexpr char chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    auto length_dist = std::uniform_int_distribution<std::size_t>{MIN_WORD_LENGTH, MAX_WORD_LENGTH};
    auto chars_dist = std::uniform_int_distribution<std::size_t>{0, sizeof(chars) - 2};
    const auto length = length_dist(rng);

    std::string result;
    result.reserve(length + 1);
    for (std::size_t i = 0; i < length; ++i)
        result.push_back(chars[chars_dist(rng)]);
    result.push_back('$');
    return result;
}

int main() {
    // Setup random generator
    std::random_device rd;
    std::mt19937 rng(rd());

    // Create trie instances
    VectorTrie v_trie;
    ArrayTrie a_trie;
    HashTrie h_trie;

    // 1) Generate random input words
    std::vector<std::string> words;
    words.reserve(NUM_WORDS);
    for (int i = 0; i < NUM_WORDS; ++i)
        words.push_back(random_word(rng));

    for (const auto &word: words) {
        auto vr = v_trie.insert(word);
        auto ar = a_trie.insert(word);
        auto hr = h_trie.insert(word);
        ASSERT_EQ(vr, ar, "[INSERT] Mismatch on word='%s'\n", word.c_str());
        ASSERT_EQ(ar, hr, "[INSERT] Mismatch on word='%s'\n", word.c_str());
    }

    // 3) Generate random queries
    // We'll pick from the existing words or some new random words
    // and random operations: { insert (i), delete (d), contains (c) }
    auto operation_dist = std::uniform_int_distribution<int>{0, 2};
    auto percent_dist = std::uniform_int_distribution<int>{0, 99};
    auto index_dist = std::uniform_int_distribution<std::size_t>(0, NUM_WORDS - 1);

    for (int i = 0; i < NUM_QUERIES; i++) {
        // pick 50% chance from existing words, 50% random new
        std::string w;
        if (percent_dist(rng) < CHANCE_RANDOM_QUERY)
            w = random_word(rng);
        else
            w = words[index_dist(rng)];

        // Perform operation on all tries
        switch (operation_dist(rng)) {
            case 0: {
                const auto vr = v_trie.insert(w);
                const auto ar = a_trie.insert(w);
                const auto hr = h_trie.insert(w);
                ASSERT_EQ(vr, ar, "[QUERY] Mismatch on operation 'insert' word='%s'\n", w.c_str());
                ASSERT_EQ(ar, hr, "[QUERY] Mismatch on operation 'insert' word='%s'\n", w.c_str());
                break;
            }
            case 1: {
                const auto vr = v_trie.remove(w);
                const auto ar = a_trie.remove(w);
                const auto hr = h_trie.remove(w);
                ASSERT_EQ(vr, ar, "[QUERY] Mismatch on operation 'remove' word='%s'\n", w.c_str());
                ASSERT_EQ(ar, hr, "[QUERY] Mismatch on operation 'remove' word='%s'\n", w.c_str());
                break;
            }
            case 2: {
                const auto vr = v_trie.contains(w);
                const auto ar = a_trie.contains(w);
                const auto hr = h_trie.contains(w);
                ASSERT_EQ(vr, ar, "[QUERY] Mismatch on operation 'contains' word='%s'\n", w.c_str());
                ASSERT_EQ(ar, hr, "[QUERY] Mismatch on operation 'contains' word='%s'\n", w.c_str());
                break;
            }
            default:
                continue;
        }
    }

    return 0;
}
