cmake_minimum_required(VERSION 3.20)
project(itch_handler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(itch_handler src/main.cpp src/order_book.cpp src/book_manager.cpp)

# Tests

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(order_book_tests tests/order_book_test.cpp src/order_book.cpp)
target_include_directories(order_book_tests PRIVATE src)
target_link_libraries(order_book_tests PRIVATE GTest::gtest_main)

add_executable(book_manager_tests tests/book_manager_test.cpp src/order_book.cpp src/book_manager.cpp)
target_include_directories(book_manager_tests PRIVATE src)
target_link_libraries(book_manager_tests PRIVATE GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(order_book_tests)
gtest_discover_tests(book_manager_tests)
