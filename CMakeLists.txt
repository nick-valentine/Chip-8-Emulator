cmake_minimum_required (VERSION 3.0)
project (chip-8-emu)

SET (GCC_COMPILE_FLAGS "-std=c++17 -g -ggdb -O0 -pedantic")

SET (CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COMPILE_FLAGS}" )

SET (THREADS_PREFER_PTHREAD_FLAG ON)
FIND_PACKAGE (Threads REQUIRED)

ADD_SUBDIRECTORY (src)

ADD_EXECUTABLE (chip-8 ${SOURCES} src/main.cpp)

# Google Test Framework
#Locate GTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

#link runtests with what we want to test
add_executable(test ${SOURCES} src/test.cpp)
target_link_libraries(test ${GTEST_LIBRARIES} pthread dl)

# Google Benchmark Framework
find_package(benchmark REQUIRED)
add_executable(benchmark ${SOURCES} src/benchmark.cpp)
target_link_libraries(benchmark pthread dl benchmark::benchmark)
