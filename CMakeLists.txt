cmake_minimum_required(VERSION 3.16)
project(order_matching_engine CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(main
    src/main.cpp
    src/book.cpp
    src/sequencer.cpp
)
target_include_directories(main PRIVATE include)

add_executable(test_replay
    tests/test_replay.cpp
    src/book.cpp
    src/sequencer.cpp
)
target_include_directories(test_replay PRIVATE include)