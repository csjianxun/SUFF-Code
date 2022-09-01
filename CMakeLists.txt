cmake_minimum_required(VERSION 2.8.12)
if (APPLE)
    set(CMAKE_C_COMPILER "/usr/local/Cellar/gcc/11.3.0/bin/gcc-11")
    set(CMAKE_CXX_COMPILER "/usr/local/Cellar/gcc/11.3.0/bin/g++-11")
elseif (EXISTS "/usr/local/bin/gcc11")  # lccpuXX env
    set(CMAKE_C_COMPILER "/usr/local/bin/gcc11")
    set(CMAKE_CXX_COMPILER "/usr/local/bin/g++11")
else ()
    set(CMAKE_C_COMPILER "gcc-11")
    set(CMAKE_CXX_COMPILER "g++-11")
    link_libraries(stdc++fs)
endif ()

project(SubgraphMatching)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a -O3 -Wall -march=native -pthread")

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_LIST_DIR}/bin)

include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/src/
)

if (APPLE)
    include_directories(/usr/local/Cellar/gperftools/2.9.1_1/include)
    # LIST(APPEND CMAKE_MODULE_PATH "/Users/jianxun/CLionProjects/Graph")
else()
    include_directories(/usr/include)
    # LIST(APPEND CMAKE_MODULE_PATH "/data")
endif()

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR};${CMAKE_MODULE_PATH}")
find_package(Gperftools REQUIRED)
message("tcmalloc and profiler lib: ${GPERFTOOLS_LIBRARIES}")

add_subdirectory(src)
