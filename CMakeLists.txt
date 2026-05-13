cmake_minimum_required(VERSION 3.20)
project(itch_handler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(
    itch_handler
    src/main.cpp
    src/order_book.cpp
    src/book_manager.cpp
    src/csv/csv_writer.cpp
    src/symbol_loader.cpp
)
target_include_directories(itch_handler PRIVATE src)

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

add_executable(csv_writer_tests tests/csv_writer_test.cpp src/csv/csv_writer.cpp src/order_book.cpp)
target_include_directories(csv_writer_tests PRIVATE src)
target_link_libraries(csv_writer_tests PRIVATE GTest::gtest_main)

add_executable(symbol_loader_tests tests/symbol_loader_test.cpp src/symbol_loader.cpp)
target_include_directories(symbol_loader_tests PRIVATE src)
target_link_libraries(symbol_loader_tests PRIVATE GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(order_book_tests)
gtest_discover_tests(book_manager_tests)
gtest_discover_tests(csv_writer_tests)
gtest_discover_tests(symbol_loader_tests)
