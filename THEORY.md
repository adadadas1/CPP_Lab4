# Теория к лабораторной работе №4: идея и реализация аллокаторов

## Цель

Получить навыки работы с аллокаторами в языке программирования C++.

---

## Управление памятью в C++: разделение аллокации и инициализации

В C++ важно различать два отдельных процесса:
1. **Выделение памяти** — получить байты от ОС.
2. **Конструирование объекта** — вызвать конструктор в уже выделенной памяти.

```cpp
// Обычный new: сразу оба действия
MyClass* p = new MyClass(42);

// Разделение:
void* mem = ::operator new(sizeof(MyClass));  // только память
MyClass* p = new(mem) MyClass(42);             // placement new — только конструктор

// Уничтожение:
p->~MyClass();   // только деструктор
::operator delete(mem);  // только освобождение памяти
```

Эта концепция — основа работы аллокаторов.

---

## Зачем нужны пользовательские аллокаторы

Стандартный `new` внутри вызывает `malloc` — системный вызов. При частых мелких аллокациях:

- появляется **фрагментация памяти**: свободные байты есть, но не непрерывно;
- растёт накладной расход на системные вызовы;
- становится сложно трассировать потребление памяти.

### Пример фрагментации

```
[занято][свободно][занято][свободно]
```

Если нужен непрерывный блок из двух байт — его нет, хотя суммарно два свободных байта есть.

### Решение — Memory Pool

Заранее выделяем большой кусок памяти, нарезаем его на блоки равного размера и раздаём из пула. Системных вызовов — минимум.

```
OS → выделяет Memory Pool (10 кб)
Pool → нарезает на блоки по 32 байта
Контейнер → запрашивает у Pool
```

---

## std::allocator и интерфейс аллокатора (C++03)

Все контейнеры STL принимают аллокатор как шаблонный параметр:

```cpp
std::vector<int>                            v;   // std::allocator<int> по умолчанию
std::vector<int, std::allocator<int>>       v;   // явно
std::vector<int, MyPoolAllocator<int>>      v;   // пользовательский
```

### Минимальный интерфейс аллокатора (C++11)

```cpp
template<typename T>
struct MyAllocator {
    using value_type = T;

    MyAllocator() noexcept {}

    template<typename U>
    MyAllocator(const MyAllocator<U>&) noexcept {}   // конструктор rebind

    T* allocate(std::size_t n) {
        return static_cast<T*>(::operator new(n * sizeof(T)));
    }

    void deallocate(T* p, std::size_t) noexcept {
        ::operator delete(p);
    }
};

template<typename T, typename U>
bool operator==(const MyAllocator<T>&, const MyAllocator<U>&) { return true; }
template<typename T, typename U>
bool operator!=(const MyAllocator<T>&, const MyAllocator<U>&) { return false; }
```

Начиная с C++11, `construct` и `destroy` можно не определять — они берутся из `std::allocator_traits` (placement new и вызов деструктора).

---

## rebind

Контейнеры вроде `std::list<T>` внутри аллоцируют не `T`, а `Node<T>`. Чтобы аллокатор от `T` умел выделять память под `Node<T>`, используется rebind:

```cpp
template<typename U>
struct rebind { using other = MyAllocator<U>; };
// или в C++11:
using other = typename std::allocator_traits<Alloc>::template rebind_alloc<Node>;
```

---

## std::allocator_traits (C++11)

Контейнер работает не напрямую с аллокатором, а через посредника — `allocator_traits`. Это даёт дефолтные реализации `construct`, `destroy` и алиасов типов:

```cpp
using Traits = std::allocator_traits<Alloc>;

T* p = Traits::allocate(alloc, 1);          // выделить
Traits::construct(alloc, p, value);         // построить
Traits::destroy(alloc, p);                  // разрушить
Traits::deallocate(alloc, p, 1);            // освободить
```

---

## Stateless vs. Stateful аллокаторы

### Stateless

Нет нестатических полей. Любые два экземпляра одного типа равнозначны — `operator==` возвращает `true`. Аллокаторы C++03 обязаны быть stateless.

```cpp
MyStatelessAlloc a1, a2;
a1 == a2;  // true — один может освободить память другого
```

### Stateful

Есть поля (например, указатель на пул). Два экземпляра могут работать с разными пулами:

```cpp
PoolAlloc a1, a2;
a1 == a2;  // false — разные пулы, нельзя мешать
```

При `swap` контейнеров до C++11 аллокаторы не менялись — могла возникнуть утечка. В C++11 это исправлено через `propagate_on_container_swap` в `allocator_traits`.

---

## Pool Allocator (пул-аллокатор)

### Идея

Заранее выделяем блок памяти на N элементов. При `allocate(1)` отдаём следующий свободный слот. При `deallocate` кладём слот в список свободных.

```
[Block: N слотов]
[0][1][2]...[N-1]
         ^
       used = 3

free_list: nullptr  (нет освобождённых)
```

После `deallocate(slot[1])`:
```
free_list → [slot[1]] → nullptr
```

При следующем `allocate(1)` берём из `free_list`:
```
free_list → nullptr
```

### Расширение

Когда текущий блок заполнен, создаётся новый блок и присоединяется в цепочку:

```
[Block 0] → [Block 1] → [Block 2]
```

### Ключевые свойства

- `allocate(n)` поддерживается только для `n == 1` (типично для узловых контейнеров).
- Освобождение памяти происходит **сразу** при уничтожении аллокатора.
- Снижает количество системных вызовов: `malloc` вызывается лишь при создании нового блока.

---

## Перегрузка operator new

Для трассировки всех аллокаций можно переопределить глобальный `operator new`:

```cpp
void* operator new(std::size_t sz) {
    std::cout << "alloc " << sz << " bytes\n";
    if (void* p = std::malloc(sz)) return p;
    throw std::bad_alloc{};
}
```

Недостаток: переопределение распространяется на весь код в программе. Аллокаторы решают эту проблему точечно — только для конкретного контейнера.

---

## Полиморфный аллокатор (C++17)

Проблема шаблонного подхода: `std::vector<int, Alloc1>` и `std::vector<int, Alloc2>` — **разные типы**, несовместимые друг с другом.

Решение — `std::pmr::polymorphic_allocator` (pmr = polymorphic memory resource):

```cpp
std::pmr::vector<int> v1;                       // стандартный new/delete
MyMemoryResource res;
std::pmr::vector<int> v2(&res);                 // пользовательский ресурс
// v1 и v2 — одного типа!
```

`memory_resource` — абстрактный базовый класс. Поведение меняется через виртуальные функции (динамический полиморфизм), а не через шаблонный параметр (статический).

```cpp
class memory_resource {
    virtual void* do_allocate(size_t bytes, size_t align)   = 0;
    virtual void  do_deallocate(void*, size_t, size_t)      = 0;
    virtual bool  do_is_equal(const memory_resource&) const = 0;
};
```

---

## Краткая сводка для защиты

### Ключевые определения

- **Аллокатор** — объект, отвечающий за выделение и освобождение памяти для контейнеров STL. Интерфейс: `allocate(n)`, `deallocate(p, n)`, `construct(p, args...)`, `destroy(p)`.
- **Пул-аллокатор (pool allocator)** — предвыделяет большой блок памяти, нарезает его на слоты. Снижает число системных вызовов `malloc/free`.
- **Stateful allocator** — хранит состояние (указатель на пул). Требует `rebind` для возможности использования с разными типами.
- **`allocator_traits`** — вспомогательный шаблон, предоставляющий стандартный интерфейс работы с аллокатором. Используется внутри контейнеров STL.
- **`rebind`** — позволяет получить аллокатор для другого типа: `PoolAllocator<T,N>::rebind<U>::other` → `PoolAllocator<U,N>`. Это нужно, например, потому что `std::map` выделяет не `T`, а `std::pair<const Key, Value>`.
- **Free list** — список освобождённых слотов. При `deallocate` слот добавляется в список; при `allocate` сначала проверяется free list.
- **Умный указатель `shared_ptr`** — счётчик ссылок, автоматически освобождает объект когда счётчик = 0.

### Как работает PoolAllocator

```
[Block: data[CHUNK*N]] → [Block: data[CHUNK*N]] → ...
         ↑ current                                  
free_list → [слот] → [слот] → nullptr
```

1. `allocate(1)`: сначала проверяет free_list; иначе берёт следующий слот в current блоке; если блок заполнен — добавляет новый блок.
2. `deallocate(p)`: добавляет слот в начало free_list.
3. Общее состояние (`State`) хранится в `shared_ptr` — поэтому копии аллокатора разделяют один пул.

### Типичные вопросы на защите

**Q: Зачем вообще нужен свой аллокатор, если есть стандартный?**  
A: Стандартный `new/malloc` может фрагментировать память и тратить время на системные вызовы. Пул-аллокатор предвыделяет память блоком — быстрее и без фрагментации.

**Q: Что такое `rebind` и зачем он нужен?**  
A: Контейнер `std::map<int,int>` внутри выделяет узлы типа `Node`, а не `int`. `rebind<Node>::other` даёт аллокатор для нужного типа из того же пула.

**Q: Почему CHUNK = max(sizeof(T), sizeof(void*))?**  
A: Свободный слот должен хранить указатель на следующий свободный слот (free list). Если `sizeof(T) < sizeof(void*)`, в слот не поместится указатель.

**Q: Чем `stateful` аллокатор отличается от `stateless`?**  
A: Stateless (например, `std::allocator`) не хранит данных, все экземпляры равнозначны. Stateful хранит указатель на пул — разные экземпляры могут управлять разными пулами.

**Q: Что происходит при `PoolAllocator(const PoolAllocator<U,N>&)`?**  
A: Создаётся новый пул (новый `State`), а не шаринг старого. Это нужно потому что типы T и U могут иметь разный размер.

**Q: Что такое `allocator_traits::construct`?**  
A: Вызывает placement new: `::new(static_cast<void*>(p)) U(args...)`. Отделяет выделение памяти от конструирования объекта.
