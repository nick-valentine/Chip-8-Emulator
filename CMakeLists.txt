cmake_minimum_required (VERSION 3.0)
project (chip-8-emu)

SET (GCC_COMPILE_FLAGS "-std=c++17 -g -ggdb -O0 -pedantic")

# @ mac only: this shouldn't be explicitely necessary
set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} /usr/local/lib)
# Add MacPorts
INCLUDE_DIRECTORIES(/usr/local/include)
LINK_DIRECTORIES(/usr/local/lib)

SET (CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COMPILE_FLAGS}" )

# SDL setup
find_file(SDL2_INCLUDE_DIR NAME SDL.h HINTS SDL2)
find_library(SDL2_LIBRARY NAME SDL2)


SET (THREADS_PREFER_PTHREAD_FLAG ON)
FIND_PACKAGE (Threads REQUIRED)

ADD_SUBDIRECTORY (src)

ADD_EXECUTABLE (chip-8 ${SOURCES} src/main.cpp)
target_link_libraries(chip-8 ${SDL2_LIBRARIES})
target_link_libraries(chip-8 SDL2)

# Google Test Framework
#Locate GTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

#link runtests with what we want to test
add_executable(test ${SOURCES} src/test.cpp)
target_link_libraries(test ${GTEST_LIBRARIES} pthread dl)
target_link_libraries(test ${SDL2_LIBRARIES})
target_link_libraries(test SDL2)

# Google Benchmark Framework
find_package(benchmark REQUIRED)
add_executable(benchmark ${SOURCES} src/benchmark.cpp)
target_link_libraries(benchmark pthread dl benchmark::benchmark)
target_link_libraries(benchmark ${SDL2_LIBRARIES})
target_link_libraries(benchmark SDL2)
