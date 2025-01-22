#pragma once

#include <memory>
#include <algorithm>

class ArrayTrie {
private:
    struct Node {
        bool is_end = false;
        std::unique_ptr<Node> children['z' + 1];

        Node() : is_end(false) {}
    };

    std::unique_ptr<Node> root;

public:
    ArrayTrie() : root(std::make_unique<Node>()) {}

    [[nodiscard]] bool insert(const std::string &word) {
        Node *curr = root.get();
        bool insertedNewNode = false;

        for (char c: word) {
            auto uc = static_cast<unsigned char> (c);

            if (!curr->children[uc]) {
                curr->children[uc] = std::make_unique<Node>();
                insertedNewNode = true;
            }
            curr = curr->children[uc].get();
        }
        bool wasEnd = curr->is_end;
        curr->is_end = true;
        return (!wasEnd) || insertedNewNode;
    }

    [[nodiscard]] bool contains(const std::string &word) const {
        const Node *curr = root.get();
        for (char c: word) {
            auto uc = static_cast<unsigned char> (c);
            if (!curr->children[uc]) {
                return false;
            }
            curr = curr->children[uc].get();
        }
        return curr->is_end;
    }

    [[nodiscard]] bool remove(const std::string &word) {
        return removeHelper(root.get(), word, 0);
    }

private:
    [[nodiscard]] bool removeHelper(Node *node, const std::string &word, std::size_t index) {
        if (!node) return false;
        if (index == word.size()) {
            if (!node->is_end) return false;
            node->is_end = false;
            // Check if all children are null
            return allChildrenNull(node);
        }
        auto uc = static_cast<unsigned char> (word[index]);
        if (!node->children[uc]) {
            return false;
        }
        bool shouldPrune = removeHelper(node->children[uc].get(), word, index + 1);
        if (shouldPrune) {
            node->children[uc].reset(nullptr);
            // If node is not an endpoint, check if we can prune further
            return (!node->is_end && allChildrenNull(node));
        }
        return false;
    }

    static bool allChildrenNull(const Node *node) {
        for (auto &child: node->children)
            if (child) return false;
        return true;
    }
};
