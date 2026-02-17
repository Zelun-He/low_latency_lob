#pragma once
/// --------------------------------------------------------
/// IntrusiveList<T> — Doubly-linked list with no allocations
///
/// Requirements on T:
///   T* next;
///   T* prev;
///
/// • O(1) push_back, pop_front, remove
/// • Zero heap allocation (nodes carry their own pointers)
/// • Perfect for price-level order queues
/// --------------------------------------------------------

#include <cstddef>

namespace lob {

template <typename T>
class IntrusiveList {
public:
    IntrusiveList() = default;

    // Non-copyable (nodes are shared state)
    IntrusiveList(const IntrusiveList&) = delete;
    IntrusiveList& operator=(const IntrusiveList&) = delete;

    // Movable
    IntrusiveList(IntrusiveList&& o) noexcept
        : head_(o.head_), tail_(o.tail_), size_(o.size_) {
        o.head_ = o.tail_ = nullptr;
        o.size_ = 0;
    }
    IntrusiveList& operator=(IntrusiveList&& o) noexcept {
        head_ = o.head_;  tail_ = o.tail_;  size_ = o.size_;
        o.head_ = o.tail_ = nullptr;  o.size_ = 0;
        return *this;
    }

    /// Append node at the back (FIFO order — price-time priority).
    void push_back(T* node) noexcept {
        node->next = nullptr;
        node->prev = tail_;
        if (tail_) {
            tail_->next = node;
        } else {
            head_ = node;
        }
        tail_ = node;
        ++size_;
    }

    /// Remove and return the front node (oldest order at this level).
    T* pop_front() noexcept {
        if (!head_) return nullptr;
        T* node = head_;
        head_ = head_->next;
        if (head_) {
            head_->prev = nullptr;
        } else {
            tail_ = nullptr;
        }
        node->next = nullptr;
        node->prev = nullptr;
        --size_;
        return node;
    }

    /// Remove an arbitrary node (e.g. cancel).
    void remove(T* node) noexcept {
        if (node->prev) {
            node->prev->next = node->next;
        } else {
            head_ = node->next;
        }
        if (node->next) {
            node->next->prev = node->prev;
        } else {
            tail_ = node->prev;
        }
        node->next = nullptr;
        node->prev = nullptr;
        --size_;
    }

    T*          front() const noexcept { return head_; }
    bool        empty() const noexcept { return head_ == nullptr; }
    std::size_t size()  const noexcept { return size_; }

private:
    T* head_ = nullptr;
    T* tail_ = nullptr;
    std::size_t size_ = 0;
};

} // namespace lob
