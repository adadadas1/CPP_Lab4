#include "pool_allocator.h"
#include "my_container.h"

#include <iostream>
#include <map>

// Факториал числа n
int factorial(int n)
{
    int result = 1;
    for (int i = 2; i <= n; ++i)
        result *= i;
    return result;
}

// Печатаем содержимое std::map
template <typename Map>
void print_map(const Map& m)
{
    for (const auto& kv : m)
        std::cout << kv.first << " " << kv.second << "\n";
}

// Печатаем содержимое MyContainer
template <typename Container>
void print_container(const Container& c)
{
    for (const auto& v : c)
        std::cout << v << "\n";
}

int main()
{
    // Шаг 1: std::map со стандартным аллокатором
    std::cout << "=== std::map (стандартный аллокатор) ===\n";
    std::map<int, int> map_std;

    // Шаг 2: заполняем 10 элементами: ключ i, значение i!
    for (int i = 0; i <= 9; ++i)
        map_std[i] = factorial(i);

    // Шаг 3: создаём std::map с пул-аллокатором (пул на 10 пар)
    std::cout << "\n=== std::map (пул-аллокатор) ===\n";
    using PairAlloc = PoolAllocator<std::pair<const int, int>, 10>;
    std::map<int, int, std::less<int>, PairAlloc> map_pool;

    // Шаг 4: заполняем 10 элементами: ключ i, значение i!
    for (int i = 0; i <= 9; ++i)
        map_pool[i] = factorial(i);

    // Шаг 5: выводим содержимое map с пул-аллокатором
    print_map(map_pool);

    // Шаг 6: создаём MyContainer со стандартным аллокатором
    std::cout << "\n=== MyContainer (стандартный аллокатор) ===\n";
    MyContainer<int> cont_std;

    // Шаг 7: заполняем 10 элементами от 0 до 9
    for (int i = 0; i <= 9; ++i)
        cont_std.push_back(i);

    // Шаг 8: создаём MyContainer с пул-аллокатором (пул на 10 int)
    std::cout << "\n=== MyContainer (пул-аллокатор) ===\n";
    using IntAlloc = PoolAllocator<int, 10>;
    MyContainer<int, IntAlloc> cont_pool;

    // Шаг 9: заполняем 10 элементами от 0 до 9
    for (int i = 0; i <= 9; ++i)
        cont_pool.push_back(i);

    // Шаг 10: выводим содержимое контейнера с пул-аллокатором
    print_container(cont_pool);

    return 0;
}
