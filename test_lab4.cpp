#define BOOST_TEST_MODULE test_lab4

#include "pool_allocator.h"
#include "my_container.h"

#include <boost/test/unit_test.hpp>
#include <map>
#include <vector>

BOOST_AUTO_TEST_SUITE(test_pool_allocator)

// Аллокатор выдаёт корректные указатели и не падает
BOOST_AUTO_TEST_CASE(test_allocate_deallocate)
{
    PoolAllocator<int, 5> alloc;

    int* p1 = alloc.allocate(1);
    alloc.construct(p1, 42);
    BOOST_CHECK_EQUAL(*p1, 42);

    int* p2 = alloc.allocate(1);
    alloc.construct(p2, 100);
    BOOST_CHECK_EQUAL(*p2, 100);

    alloc.destroy(p1);
    alloc.deallocate(p1, 1);
    alloc.destroy(p2);
    alloc.deallocate(p2, 1);
}

// Пул автоматически расширяется, когда заканчиваются слоты
BOOST_AUTO_TEST_CASE(test_pool_expansion)
{
    PoolAllocator<int, 3> alloc;  // только 3 слота в первом блоке

    std::vector<int*> ptrs;
    for (int i = 0; i < 7; ++i) {  // 7 > 3, значит будет расширение
        int* p = alloc.allocate(1);
        alloc.construct(p, i * 10);
        ptrs.push_back(p);
    }

    for (int i = 0; i < 7; ++i)
        BOOST_CHECK_EQUAL(*ptrs[i], i * 10);

    for (int* p : ptrs) {
        alloc.destroy(p);
        alloc.deallocate(p, 1);
    }
}

// Освобождённые слоты повторно используются
BOOST_AUTO_TEST_CASE(test_free_list_reuse)
{
    PoolAllocator<int, 5> alloc;

    int* p = alloc.allocate(1);
    alloc.construct(p, 7);
    alloc.destroy(p);
    alloc.deallocate(p, 1);

    // После освобождения следующее выделение должно вернуть тот же адрес
    int* p2 = alloc.allocate(1);
    BOOST_CHECK_EQUAL(p, p2);  // адрес совпадает — слот переиспользован

    alloc.destroy(p2);
    alloc.deallocate(p2, 1);
}

// std::map с пул-аллокатором работает корректно
BOOST_AUTO_TEST_CASE(test_map_with_pool_alloc)
{
    using PairAlloc = PoolAllocator<std::pair<const int, int>, 10>;
    std::map<int, int, std::less<int>, PairAlloc> m;

    for (int i = 0; i < 5; ++i)
        m[i] = i * i;

    BOOST_REQUIRE_EQUAL(m.size(), 5u);
    BOOST_CHECK_EQUAL(m[0], 0);
    BOOST_CHECK_EQUAL(m[2], 4);
    BOOST_CHECK_EQUAL(m[4], 16);
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(test_my_container)

// push_back и итерация работают корректно
BOOST_AUTO_TEST_CASE(test_push_back_and_iterate)
{
    MyContainer<int> c;
    for (int i = 0; i < 5; ++i)
        c.push_back(i);

    BOOST_CHECK_EQUAL(c.size(), 5u);

    int expected = 0;
    for (auto v : c)
        BOOST_CHECK_EQUAL(v, expected++);
}

// empty() и size() корректны
BOOST_AUTO_TEST_CASE(test_empty_and_size)
{
    MyContainer<int> c;
    BOOST_CHECK(c.empty());
    BOOST_CHECK_EQUAL(c.size(), 0u);

    c.push_back(1);
    BOOST_CHECK(!c.empty());
    BOOST_CHECK_EQUAL(c.size(), 1u);
}

// MyContainer с пул-аллокатором работает корректно
BOOST_AUTO_TEST_CASE(test_container_with_pool_alloc)
{
    using IntAlloc = PoolAllocator<int, 10>;
    MyContainer<int, IntAlloc> c;

    for (int i = 0; i <= 9; ++i)
        c.push_back(i);

    BOOST_REQUIRE_EQUAL(c.size(), 10u);

    int expected = 0;
    for (auto v : c)
        BOOST_CHECK_EQUAL(v, expected++);
}

// Перемещение контейнера: данные переходят, источник пуст
BOOST_AUTO_TEST_CASE(test_move_semantics)
{
    MyContainer<int> src;
    src.push_back(10);
    src.push_back(20);
    src.push_back(30);

    MyContainer<int> dst = std::move(src);

    BOOST_CHECK(src.empty());
    BOOST_REQUIRE_EQUAL(dst.size(), 3u);

    auto it = dst.begin();
    BOOST_CHECK_EQUAL(*it++, 10);
    BOOST_CHECK_EQUAL(*it++, 20);
    BOOST_CHECK_EQUAL(*it,   30);
}

BOOST_AUTO_TEST_SUITE_END()
