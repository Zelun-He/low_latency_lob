#pragma once
/// --------------------------------------------------------
/// ObjectPool<T> — Fixed-block pool allocator
///
/// • Pre-allocates blocks of N objects on the heap
/// • O(1) allocate / deallocate via embedded free-list
/// • Zero heap allocs during steady-state operation
/// • Tracks allocated count + total capacity for metrics
/// --------------------------------------------------------

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace lob {

template <typename T, std::size_t BlockSize = 4096>
class ObjectPool {
    static_assert(sizeof(T) >= sizeof(void*),
                  "T must be at least pointer-sized for free-list embedding");

    struct FreeNode {
        FreeNode* next;
    };

public:
    ObjectPool() { grow(); }

    // Non-copyable, non-movable (pointers into our blocks are live)
    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;

    /// Grab one T from the pool.  Grows automatically if exhausted.
    [[nodiscard]] T* allocate() {
        if (!free_list_) {
            grow();
        }
        auto* slot = free_list_;
        free_list_ = slot->next;
        ++allocated_;
        return new (slot) T{};
    }

    /// Return a T to the pool (destructs, then recycles the slot).
    void deallocate(T* ptr) noexcept {
        ptr->~T();
        auto* node = reinterpret_cast<FreeNode*>(ptr);
        node->next = free_list_;
        free_list_ = node;
        --allocated_;
    }

    // ---- Metrics ----
    std::size_t allocated()   const noexcept { return allocated_; }
    std::size_t capacity()    const noexcept { return capacity_; }
    std::size_t memory_bytes() const noexcept {
        return blocks_.size() * BlockSize * sizeof(T);
    }

private:
    void grow() {
        // Allocate a raw byte block, properly aligned for T
        auto block = std::make_unique<
            std::byte[]>(BlockSize * sizeof(T));
        auto* base = block.get();

        // Thread every slot into the free list (reverse order so that
        // the first slot is at the head — improves cache locality for
        // sequential allocs)
        for (std::size_t i = BlockSize; i > 0; --i) {
            auto* node = reinterpret_cast<FreeNode*>(
                base + (i - 1) * sizeof(T));
            node->next = free_list_;
            free_list_ = node;
        }
        capacity_ += BlockSize;
        blocks_.push_back(std::move(block));
    }

    std::vector<std::unique_ptr<std::byte[]>> blocks_;
    FreeNode* free_list_ = nullptr;
    std::size_t allocated_ = 0;
    std::size_t capacity_  = 0;
};

} // namespace lob
