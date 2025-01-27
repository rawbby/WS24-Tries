#pragma once

#include <cassert> // (optional) for static_assert
#include <cstddef> // for std::size_t
#include <memory>  // for std::unique_ptr, std::make_unique
#include <string>  // for std::string

namespace util {
constexpr unsigned char
index(char c)
{
  if (c >= '0' && c <= '9')
    return static_cast<unsigned char>(c - '0' + 1);
  if (c >= 'A' && c <= 'Z')
    return static_cast<unsigned char>(c - 'A' + 11);
  if (c >= 'a' && c <= 'z')
    return static_cast<unsigned char>(c - 'a' + 37);
  return 0;
}

constexpr char
symbol(unsigned char uc)
{
  if (uc < 1)
    return 0;
  if (uc < 11)
    return '0' + static_cast<char>(uc - 1);
  if (uc < 37)
    return 'A' + static_cast<char>(uc - 11);
  if (uc < 63)
    return 'a' + static_cast<char>(uc - 37);
  return static_cast<char>(-0xff);
}
}

class ArrayTrie
{
private:
  struct Node
  {
    std::size_t is_end = false;
    std::unique_ptr<Node> children[63];

    Node()
    {
      // we make is_end a size type
      // to ensure even size of Node
      static_assert(sizeof(Node) == 64 * 8);
    }
  };

  std::unique_ptr<Node> root;

public:
  ArrayTrie()
    : root(std::make_unique<Node>())
  {
  }

  [[nodiscard]] bool insert(const std::string& word)
  {
    Node* curr = root.get();
    bool insertedNewNode = false;

    for (char c : word) {
      const auto uc = util::index(c);

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

  [[nodiscard]] bool contains(const std::string& word) const
  {
    const Node* curr = root.get();
    for (char c : word) {
      const auto uc = util::index(c);
      if (!curr->children[uc]) {
        return false;
      }
      curr = curr->children[uc].get();
    }
    return curr->is_end;
  }

  [[nodiscard]] bool remove(const std::string& word) { return removeHelper(root.get(), word, 0); }

  [[nodiscard]] std::size_t size() const { return sizeHelper(root.get()); }

private:
  [[nodiscard]] bool removeHelper(Node* node, const std::string& word, std::size_t index)
  {
    if (!node)
      return false;
    if (index == word.size()) {
      if (!node->is_end)
        return false;
      node->is_end = false;
      // Check if all children are null
      return allChildrenNull(node);
    }
    const auto uc = util::index(word[index]);
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

  static bool allChildrenNull(const Node* node)
  {
    for (auto& child : node->children)
      if (child)
        return false;
    return true;
  }

  [[nodiscard]] std::size_t sizeHelper(const Node* node) const
  {
    if (!node) {
      return 0;
    }
    std::size_t total = sizeof(*node);
    for (auto& child : node->children)
      total += sizeHelper(child.get());
    return total;
  }
};
