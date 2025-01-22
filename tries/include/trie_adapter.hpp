#pragma once

#include <string>

class TrieInterface {
public:
    virtual ~TrieInterface() = default;

    [[nodiscard]] virtual bool insert(const std::string &) = 0;

    [[nodiscard]] virtual bool contains(const std::string &) const = 0;

    [[nodiscard]] virtual bool remove(const std::string &) = 0;
};

template<typename T>
class TrieAdapter : public TrieInterface {
private:
    T trie;
public:
    [[nodiscard]] bool insert(const std::string &w) override { return trie.insert(w); }

    [[nodiscard]] bool contains(const std::string &w) const override { return trie.contains(w); }

    [[nodiscard]] bool remove(const std::string &w) override { return trie.remove(w); }
};
