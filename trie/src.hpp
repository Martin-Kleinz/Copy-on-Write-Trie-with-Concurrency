/*
 * Acknowledgement : Yuxuan Wang for Modifying the prototype of TrieStore class
 */

#ifndef SJTU_TRIE_HPP
#define SJTU_TRIE_HPP

#include <algorithm>
#include <cstddef>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace sjtu {

// A TrieNode is a node in a Trie.
class TrieNode {
   public:
    // Create a TrieNode with no children.
    TrieNode() = default;

    // Create a TrieNode with some children.
    explicit TrieNode(std::map<char, std::shared_ptr<TrieNode>> children)
        : children_(std::move(children)) {}

    virtual ~TrieNode() = default;

    // Clone returns a copy of this TrieNode. If the TrieNode has a value, the
    // value is copied. The return type of this function is a unique_ptr to a
    // TrieNode.
    //
    // You cannot use the copy constructor to clone the node because it doesn't
    // know whether a `TrieNode` contains a value or not.
    //
    // Note: if you want to convert `unique_ptr` into `shared_ptr`, you can use
    // `std::shared_ptr<T>(std::move(ptr))`.
    virtual auto Clone() const -> std::unique_ptr<TrieNode> {
        return std::make_unique<TrieNode>(children_);
    }

    // A map of children, where the key is the next character in the key, and
    // the value is the next TrieNode.
    std::map<char, std::shared_ptr<TrieNode>> children_;

    // Indicates if the node is the terminal node.
    bool is_value_node_{false};

    // You can add additional fields and methods here. But in general, you don't
    // need to add extra fields to complete this project.
};

// A TrieNodeWithValue is a TrieNode that also has a value of type T associated
// with it.
template <class T>
class TrieNodeWithValue : public TrieNode {
   public:
    // Create a trie node with no children and a value.
    explicit TrieNodeWithValue(std::shared_ptr<T> value)
        : value_(std::move(value)) {
        this->is_value_node_ = true;
    }

    // Create a trie node with children and a value.
    TrieNodeWithValue(std::map<char, std::shared_ptr<TrieNode>> children,
                      std::shared_ptr<T> value)
        : TrieNode(std::move(children)), value_(std::move(value)) {
        this->is_value_node_ = true;
    }

    // Override the Clone method to also clone the value.
    //
    // Note: if you want to convert `unique_ptr` into `shared_ptr`, you can use
    // `std::shared_ptr<T>(std::move(ptr))`.
    auto Clone() const -> std::unique_ptr<TrieNode> override {
        return std::make_unique<TrieNodeWithValue<T>>(children_, value_);
    }

    // The value associated with this trie node.
    std::shared_ptr<T> value_;
};

// A Trie is a data structure that maps strings to values of type T. All
// operations on a Trie should not modify the trie itself. It should reuse the
// existing nodes as much as possible, and create new nodes to represent the new
// trie.
class Trie {
   private:
    // The root of the trie.
    std::shared_ptr<TrieNode> root_{nullptr};

    // Create a new trie with the given root.
    explicit Trie(std::shared_ptr<TrieNode> root)
        : root_(std::move(root)) {}

   public:
    // Create an empty trie.
    Trie() = default;

    bool operator==(const Trie& other) const
    {
        return root_ == other.root_;
    }
    // by TA: if you don't need this, just comment out.

    // Get the value associated with the given key.
    // 1. If the key is not in the trie, return nullptr.
    // 2. If the key is in the trie but the type is mismatched, return nullptr.
    // 3. Otherwise, return the value.
    template <class T>
    auto Get(std::string_view key) const -> const T*
    {
        if(!root_) return nullptr;
        int i = 0;
        auto current = root_;
        while(i < key.size() && current->children_.find(key[i]) != current->children_.end()) 
        {
            auto it = current->children_.find(key[i]);
            current = it->second;
            i++;
        }
        if(i < key.size() || !current->is_value_node_) return nullptr;
        auto target = std::dynamic_pointer_cast<TrieNodeWithValue<T>>(current);
        if (!target || !target->value_) return nullptr;
        if(typeid(target->value_) != typeid(std::shared_ptr<T>)) return nullptr;
        return target->value_.get();
    }

    // Put a new key-value pair into the trie. If the key already exists,
    // overwrite the value. Returns the new trie.
    template <class T>
    auto Put(std::string_view key, T value) const -> Trie
    {
        std::shared_ptr<TrieNode> newroot, current;
        if(!root_) newroot = std::shared_ptr<TrieNode>(new TrieNode());
        else
            newroot = std::shared_ptr<TrieNode>(std::move(root_->Clone()));
        current = newroot;
        bool exist = true;
        for(int i = 0; i < key.size() - 1; ++i)
        {
            if(exist && current->children_.find(key[i]) != current->children_.end())
            {
                current->children_[key[i]] = current->children_[key[i]]->Clone();
            }
            else 
            {
                exist = false;
                current->children_[key[i]] = std::shared_ptr<TrieNode>(new TrieNode());
            }
            current = current->children_[key[i]];
        }
        auto newval = std::make_shared<T>(std::move(value));
        if(!exist || current->children_.find(key.back()) == current->children_.end())
            current->children_[key.back()] = std::shared_ptr<TrieNodeWithValue<T>>(new TrieNodeWithValue(newval));
        else
            current->children_[key.back()] = std::shared_ptr<TrieNodeWithValue<T>>(new TrieNodeWithValue(current->children_[key.back()]->children_, newval));
        return Trie(newroot);
    }

    // Remove the key from the trie. If the key does not exist, return the
    // original trie. Otherwise, returns the new trie.
    auto Remove(std::string_view key) const -> Trie
    {
        if(!root_) return *this;
        std::shared_ptr<TrieNode> newroot = std::shared_ptr<TrieNode>(std::move(root_->Clone()));
        auto current = newroot;
        for(int i = 0; i < key.size() - 1; ++i)
        {
            if(current->children_.find(key[i]) == current->children_.end()) 
                return *this;
            current->children_[key[i]] = current->children_[key[i]]->Clone();
            current = current->children_[key[i]];
        }
        if(current->children_.find(key.back()) == current->children_.end()) return *this;
        current->children_[key.back()] = std::shared_ptr<TrieNode>(new TrieNode(current->children_[key.back()]->children_));
        return Trie(newroot);
    }
};

// This class is used to guard the value returned by the trie. It holds a
// reference to the root so that the reference to the value will not be
// invalidated.
template <class T>
class ValueGuard {
   public:
    ValueGuard(Trie root, const T& value)
        : root_(std::move(root)), value_(value) {}
    auto operator*() const -> const T& { return value_; }

   private:
    Trie root_;
    const T& value_;
};

// This class is a thread-safe wrapper around the Trie class. It provides a
// simple interface for accessing the trie. It should allow concurrent reads and
// a single write operation at the same time.
class TrieStore {
   public:
    // This function returns a ValueGuard object that holds a reference to the
    // value in the trie of the given version (default: newest version). If the
    // key does not exist in the trie, it will return std::nullopt.
    template <class T>
    auto Get(std::string_view key, size_t version = -1)
        -> std::optional<ValueGuard<T>>;

    // This function will insert the key-value pair into the trie. If the key
    // already exists in the trie, it will overwrite the value return the
    // version number after operation Hint: new version should only be visible
    // after the operation is committed(completed)
    template <class T>
    size_t Put(std::string_view key, T value);

    // This function will remove the key-value pair from the trie.
    // return the version number after operation
    // if the key does not exist, version number should not be increased
    size_t Remove(std::string_view key);

    // This function return the newest version number
    size_t get_version();

   private:
    // This mutex sequences all writes operations and allows only one write
    // operation at a time. Concurrent modifications should have the effect of
    // applying them in some sequential order
    std::mutex write_lock_;
    std::shared_mutex snapshots_lock_;

    // Stores all historical versions of trie
    // version number ranges from [0, snapshots_.size())
    std::vector<Trie> snapshots_{1};
};

}  // namespace sjtu

#endif  // SJTU_TRIE_HPP