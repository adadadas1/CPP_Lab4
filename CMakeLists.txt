cmake_minimum_required(VERSION 3.10)

set(PATCH_VERSION "1" CACHE STRING "Номер патч-версии")
project(lab4 VERSION 0.0.${PATCH_VERSION})

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

# ---- Основная программа ----
add_executable(allocator_demo main.cpp)
target_include_directories(allocator_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ---- Тесты ----
find_package(Boost CONFIG REQUIRED COMPONENTS unit_test_framework)

add_executable(test_lab4 test_lab4.cpp)
target_include_directories(test_lab4 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_lab4
    PRIVATE
        Boost::unit_test_framework
)
target_compile_definitions(test_lab4 PRIVATE BOOST_TEST_DYN_LINK)

enable_testing()
add_test(NAME test_lab4 COMMAND test_lab4)
