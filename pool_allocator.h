#pragma once

#include <cstddef>
#include <memory>
#include <stdexcept>

template <typename T, std::size_t N>
class PoolAllocator
{
public:
    using value_type      = T;
    using pointer         = T*;
    using const_pointer   = const T*;
    using reference       = T&;
    using const_reference = const T&;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;

    // Размер одного слота — максимум из размера T и размера указателя,
    // чтобы в свободном слоте можно было хранить указатель на следующий.
    static constexpr std::size_t CHUNK = sizeof(T) > sizeof(void*) ? sizeof(T) : sizeof(void*);

    template <typename U>
    struct rebind { using other = PoolAllocator<U, N>; };

private:
    struct Block
    {
        alignas(std::max_align_t) char data[CHUNK * N];
        std::size_t used = 0;
        std::unique_ptr<Block> next;
    };

    struct FreeNode { FreeNode* next; };

    struct State
    {
        std::unique_ptr<Block> head;
        Block* current = nullptr;
        FreeNode* free_list = nullptr;
    };

    std::shared_ptr<State> m_state;

    void add_block()
    {
        auto blk = std::make_unique<Block>();
        Block* raw = blk.get();
        if (!m_state->head) {
            m_state->head    = std::move(blk);
            m_state->current = raw;
        } else {
            m_state->current->next = std::move(blk);
            m_state->current       = raw;
        }
    }

public:
    PoolAllocator() : m_state(std::make_shared<State>())
    {
        add_block();
    }

    PoolAllocator(const PoolAllocator&) = default;

    template <typename U>
    explicit PoolAllocator(const PoolAllocator<U, N>&)
        : m_state(std::make_shared<State>())
    {
        add_block();
    }

    pointer allocate(size_type n)
    {
        if (n != 1)
            throw std::bad_alloc();

        if (m_state->free_list) {
            FreeNode* node = m_state->free_list;
            m_state->free_list = node->next;
            return reinterpret_cast<pointer>(node);
        }

        if (m_state->current->used >= N)
            add_block();

        char* ptr = m_state->current->data + m_state->current->used * CHUNK;
        ++m_state->current->used;
        return reinterpret_cast<pointer>(ptr);
    }

    void deallocate(pointer p, size_type /*n*/)
    {
        FreeNode* node = reinterpret_cast<FreeNode*>(p);
        node->next     = m_state->free_list;
        m_state->free_list = node;
    }

    template <typename U, typename... Args>
    void construct(U* p, Args&&... args)
    {
        ::new (static_cast<void*>(p)) U(std::forward<Args>(args)...);
    }

    template <typename U>
    void destroy(U* p)
    {
        p->~U();
    }

    bool operator==(const PoolAllocator& other) const noexcept
    {
        return m_state == other.m_state;
    }
    bool operator!=(const PoolAllocator& other) const noexcept
    {
        return !(*this == other);
    }
};
