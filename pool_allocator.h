#pragma once

#include <cstddef>
#include <memory>
#include <stdexcept>

// Пул-аллокатор: заранее выделяет N элементов блоками,
// поддерживает поэлементное освобождение через список свободных слотов.
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

    // Rebind: аллокатор для другого типа создаёт новый пул
    template <typename U>
    struct rebind { using other = PoolAllocator<U, N>; };

private:
    // Блок памяти на N слотов
    struct Block
    {
        alignas(std::max_align_t) char data[CHUNK * N];
        std::size_t used = 0;          // сколько слотов занято в этом блоке
        std::unique_ptr<Block> next;   // следующий блок (при расширении)
    };

    // Узел списка свободных слотов (хранится прямо в освобождённом слоте)
    struct FreeNode { FreeNode* next; };

    // Общее состояние: разделяется между копиями одного и того же типа T
    struct State
    {
        std::unique_ptr<Block> head;   // первый блок
        Block* current = nullptr;      // текущий (последний) блок
        FreeNode* free_list = nullptr; // список освобождённых слотов
    };

    std::shared_ptr<State> m_state;

    // Добавляем новый блок в цепочку
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
    // Конструктор по умолчанию — создаёт новый пул
    PoolAllocator() : m_state(std::make_shared<State>())
    {
        add_block();
    }

    // Копирующий конструктор — разделяем состояние (тот же тип T)
    PoolAllocator(const PoolAllocator&) = default;

    // Конструктор rebind — при смене типа создаём новый пул
    template <typename U>
    explicit PoolAllocator(const PoolAllocator<U, N>&)
        : m_state(std::make_shared<State>())
    {
        add_block();
    }

    // Выделяем память под n элементов (только n==1 поддерживается пулом)
    pointer allocate(size_type n)
    {
        if (n != 1)
            throw std::bad_alloc();

        // Сначала смотрим в список свободных слотов
        if (m_state->free_list) {
            FreeNode* node = m_state->free_list;
            m_state->free_list = node->next;
            return reinterpret_cast<pointer>(node);
        }

        // Если текущий блок заполнен — добавляем новый
        if (m_state->current->used >= N)
            add_block();

        char* ptr = m_state->current->data + m_state->current->used * CHUNK;
        ++m_state->current->used;
        return reinterpret_cast<pointer>(ptr);
    }

    // Освобождаем один элемент — кладём слот в список свободных
    void deallocate(pointer p, size_type /*n*/)
    {
        FreeNode* node = reinterpret_cast<FreeNode*>(p);
        node->next     = m_state->free_list;
        m_state->free_list = node;
    }

    // Строим объект на уже выделенной памяти
    template <typename U, typename... Args>
    void construct(U* p, Args&&... args)
    {
        ::new (static_cast<void*>(p)) U(std::forward<Args>(args)...);
    }

    // Разрушаем объект (без освобождения памяти)
    template <typename U>
    void destroy(U* p)
    {
        p->~U();
    }

    // Два аллокатора равны, если разделяют одно состояние
    bool operator==(const PoolAllocator& other) const noexcept
    {
        return m_state == other.m_state;
    }
    bool operator!=(const PoolAllocator& other) const noexcept
    {
        return !(*this == other);
    }
};
