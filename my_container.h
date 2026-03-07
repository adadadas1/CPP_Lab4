#pragma once

#include <memory>

template <typename T, typename Alloc = std::allocator<T>>
class MyContainer
{
private:
    struct Node
    {
        T     value;
        Node* next = nullptr;

        explicit Node(const T& v) : value(v) {}
        explicit Node(T&& v)      : value(std::move(v)) {}
    };

    using NodeAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<Node>;
    using Traits    = std::allocator_traits<NodeAlloc>;

    NodeAlloc  m_alloc;
    Node*      m_head  = nullptr;
    Node*      m_tail  = nullptr;
    std::size_t m_size = 0;

    Node* make_node(const T& v)
    {
        Node* p = Traits::allocate(m_alloc, 1);
        Traits::construct(m_alloc, p, v);
        return p;
    }
    Node* make_node(T&& v)
    {
        Node* p = Traits::allocate(m_alloc, 1);
        Traits::construct(m_alloc, p, std::move(v));
        return p;
    }

    void free_node(Node* p)
    {
        Traits::destroy(m_alloc, p);
        Traits::deallocate(m_alloc, p, 1);
    }

public:
    explicit MyContainer(const Alloc& alloc = Alloc())
        : m_alloc(alloc) {}

    MyContainer(MyContainer&& other) noexcept
        : m_alloc(std::move(other.m_alloc))
        , m_head(other.m_head)
        , m_tail(other.m_tail)
        , m_size(other.m_size)
    {
        other.m_head = nullptr;
        other.m_tail = nullptr;
        other.m_size = 0;
    }

    MyContainer& operator=(MyContainer&& other) noexcept
    {
        if (this != &other) {
            clear();
            m_alloc      = std::move(other.m_alloc);
            m_head       = other.m_head;
            m_tail       = other.m_tail;
            m_size       = other.m_size;
            other.m_head = nullptr;
            other.m_tail = nullptr;
            other.m_size = 0;
        }
        return *this;
    }

    MyContainer(const MyContainer&)            = delete;
    MyContainer& operator=(const MyContainer&) = delete;

    ~MyContainer() { clear(); }

    void push_back(const T& v)
    {
        Node* n = make_node(v);
        if (!m_tail) {
            m_head = m_tail = n;
        } else {
            m_tail->next = n;
            m_tail       = n;
        }
        ++m_size;
    }

    void push_back(T&& v)
    {
        Node* n = make_node(std::move(v));
        if (!m_tail) {
            m_head = m_tail = n;
        } else {
            m_tail->next = n;
            m_tail       = n;
        }
        ++m_size;
    }

    std::size_t size()  const noexcept { return m_size; }
    bool        empty() const noexcept { return m_size == 0; }

    void clear()
    {
        Node* cur = m_head;
        while (cur) {
            Node* next = cur->next;
            free_node(cur);
            cur = next;
        }
        m_head = m_tail = nullptr;
        m_size = 0;
    }

    class Iterator
    {
        Node* m_ptr;
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = T*;
        using reference         = T&;

        explicit Iterator(Node* p) : m_ptr(p) {}

        reference operator*()  const { return m_ptr->value; }
        pointer   operator->() const { return &m_ptr->value; }

        Iterator& operator++()
        {
            m_ptr = m_ptr->next;
            return *this;
        }
        Iterator operator++(int)
        {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const Iterator& o) const { return m_ptr == o.m_ptr; }
        bool operator!=(const Iterator& o) const { return m_ptr != o.m_ptr; }
    };

    class ConstIterator
    {
        const Node* m_ptr;
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = const T*;
        using reference         = const T&;

        explicit ConstIterator(const Node* p) : m_ptr(p) {}

        reference operator*()  const { return m_ptr->value; }
        pointer   operator->() const { return &m_ptr->value; }

        ConstIterator& operator++()
        {
            m_ptr = m_ptr->next;
            return *this;
        }
        ConstIterator operator++(int)
        {
            ConstIterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const ConstIterator& o) const { return m_ptr == o.m_ptr; }
        bool operator!=(const ConstIterator& o) const { return m_ptr != o.m_ptr; }
    };

    Iterator      begin()        { return Iterator(m_head); }
    Iterator      end()          { return Iterator(nullptr); }
    ConstIterator begin()  const { return ConstIterator(m_head); }
    ConstIterator end()    const { return ConstIterator(nullptr); }
    ConstIterator cbegin() const { return ConstIterator(m_head); }
    ConstIterator cend()   const { return ConstIterator(nullptr); }
};
