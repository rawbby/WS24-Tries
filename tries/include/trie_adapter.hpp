#pragma once

#include <cstddef> // for std::size_t
#include <string>  // for std::string

class TrieInterface
{
public:
  virtual ~TrieInterface() = default;

  [[nodiscard]] virtual bool insert(const std::string&) = 0;

  [[nodiscard]] virtual bool contains(const std::string&) const = 0;

  [[nodiscard]] virtual bool remove(const std::string&) = 0;

  [[nodiscard]] virtual std::size_t size() const = 0;
};

template<typename T>
class TrieAdapter : public TrieInterface
{
private:
  T trie;

public:
  [[nodiscard]] bool insert(const std::string& w) override { return trie.insert(w); }

  [[nodiscard]] bool contains(const std::string& w) const override { return trie.contains(w); }

  [[nodiscard]] bool remove(const std::string& w) override { return trie.remove(w); }

  [[nodiscard]] std::size_t size() const override { return trie.size(); }
};
