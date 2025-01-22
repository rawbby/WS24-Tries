#pragma once

#include <vector>
#include <memory>
#include <algorithm>

class VectorTrie {
private:
    struct Node {
        bool is_end = false;
        std::vector<std::pair<unsigned char, std::unique_ptr<Node>>> children;
    };

    std::unique_ptr<Node> root;

public:
    VectorTrie() : root(std::make_unique<Node>()) {}

    // Insert a word (excluding trailing 0-byte or '$')
    bool insert(const std::string &word) {
        Node *curr = root.get();
        bool insertedNewNode = false;

        for (char c: word) {
            auto uc = static_cast<unsigned char>(c);

            // Search in curr->children for c
            auto it = std::find_if(curr->children.begin(), curr->children.end(),
                                   [uc](auto &p) { return p.first == uc; });
            if (it == curr->children.end()) {
                // Not found -> create new child
                curr->children.push_back({c, std::make_unique<Node>()});
                curr = curr->children.back().second.get();
                insertedNewNode = true;
            } else {
                // Found existing
                curr = it->second.get();
            }
        }
        // Mark end of word
        bool wasEnd = curr->is_end;
        curr->is_end = true;
        // Return true if this insertion made a "new" word, false if it already existed
        return (!wasEnd) || insertedNewNode;
    }

    // Check if word is contained
    bool contains(const std::string &word) const {
        const Node *curr = root.get();
        for (char c: word) {
            auto uc = static_cast<unsigned char>(c);
            auto it = std::find_if(curr->children.begin(), curr->children.end(),
                                   [uc](auto &p) { return p.first == uc; });
            if (it == curr->children.end()) {
                return false;
            }
            curr = it->second.get();
        }
        return curr->is_end;
    }

    // Remove a word (return true if removal was successful)
    bool remove(const std::string &word) {
        return removeHelper(root.get(), word, 0);
    }

private:
    // Recursive helper for remove
    bool removeHelper(Node *node, const std::string &word, size_t index) {
        if (!node) return false;
        if (index == word.size()) {
            if (!node->is_end) return false; // not found
            node->is_end = false;
            // Return true if this node has no children (caller can prune)
            return node->children.empty();
        }
        auto c = static_cast<unsigned char> (word[index]);
        auto it = std::find_if(node->children.begin(), node->children.end(),
                               [c](auto &p) { return p.first == c; });
        if (it == node->children.end()) {
            return false;
        }
        Node *childNode = it->second.get();
        bool shouldPrune = removeHelper(childNode, word, index + 1);

        if (shouldPrune) {
            // remove the pair from the vector
            node->children.erase(it);
            // Return true if node has no children and is not end
            return (node->children.empty() && !node->is_end);
        }
        return false;
    }
};
