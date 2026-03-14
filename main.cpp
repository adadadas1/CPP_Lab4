#include "pool_allocator.h"
#include "my_container.h"

#include <iostream>
#include <map>

int factorial(int n)
{
    int result = 1;
    for (int i = 2; i <= n; ++i)
        result *= i;
    return result;
}

template <typename Map>
void print_map(const Map& m)
{
    for (const auto& kv : m)
        std::cout << kv.first << " " << kv.second << "\n";
}

template <typename Container>
void print_container(const Container& c)
{
    for (const auto& value : c)
        std::cout << value << "\n";
}

int main()
{
    std::cout << "=== std::map (стандартный аллокатор) ===\n";
    std::map<int, int> map_std;
    for (int i = 0; i <= 9; ++i)
        map_std[i] = factorial(i);

    std::cout << "\n=== std::map (пул-аллокатор) ===\n";
    using PairAlloc = PoolAllocator<std::pair<const int, int>, 10>;
    std::map<int, int, std::less<int>, PairAlloc> map_pool;
    for (int i = 0; i <= 9; ++i)
        map_pool[i] = factorial(i);
    print_map(map_pool);

    std::cout << "\n=== MyContainer (стандартный аллокатор) ===\n";
    MyContainer<int> cont_std;
    for (int i = 0; i <= 9; ++i)
        cont_std.push_back(i);

    std::cout << "\n=== MyContainer (пул-аллокатор) ===\n";
    using IntAlloc = PoolAllocator<int, 10>;
    MyContainer<int, IntAlloc> cont_pool;
    for (int i = 0; i <= 9; ++i)
        cont_pool.push_back(i);
    print_container(cont_pool);

    return 0;
}
