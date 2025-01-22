#pragma once

#include <memory>
#include <unordered_map>

class HashTrie {
private:
    struct Node {
        bool is_end = false;
        std::unordered_map<unsigned char, std::unique_ptr<Node>> children;
    };

    std::unique_ptr<Node> root;

public:
    HashTrie() : root(std::make_unique<Node>()) {}

    bool insert(const std::string &word) {
        Node *curr = root.get();
        bool insertedNewNode = false;

        for (char c: word) {
            auto uc = static_cast<unsigned char>(c);

            auto it = curr->children.find(uc);
            if (it == curr->children.end()) {
                curr->children[uc] = std::make_unique<Node>();
                insertedNewNode = true;
                curr = curr->children[uc].get();
            } else {
                curr = it->second.get();
            }
        }
        bool wasEnd = curr->is_end;
        curr->is_end = true;
        return (!wasEnd) || insertedNewNode;
    }

    bool contains(const std::string &word) const {
        const Node *curr = root.get();
        for (char c: word) {
            auto uc = static_cast<unsigned char>(c);
            auto it = curr->children.find(uc);
            if (it == curr->children.end())
                return false;
            curr = it->second.get();
        }
        return curr->is_end;
    }

    bool remove(const std::string &word) {
        return removeHelper(root.get(), word, 0);
    }

private:
    bool removeHelper(Node *node, const std::string &word, size_t index) {
        if (!node) return false;
        if (index == word.size()) {
            if (!node->is_end) return false;
            node->is_end = false;
            return node->children.empty();
        }
        auto uc = static_cast<unsigned char>(word[index]);
        auto it = node->children.find(uc);
        if (it == node->children.end()) {
            return false;
        }
        bool shouldPrune = removeHelper(it->second.get(), word, index + 1);
        if (shouldPrune) {
            node->children.erase(uc);
            return (!node->is_end && node->children.empty());
        }
        return false;
    }
};
