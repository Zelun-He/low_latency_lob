cmake_minimum_required(VERSION 3.20)
project(low_latency_lob LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(lob_engine
    src/main.cpp
    src/order_book.cpp
)

if (MSVC)
    target_compile_options(lob_engine PRIVATE /O2)
else()
    target_compile_options(lob_engine PRIVATE -O3)
endif()
