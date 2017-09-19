cmake_minimum_required(VERSION 2.6)
project(Task_Scheduler)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -pg -ggdb -std=c++14 -pedantic")
set(warnings "-Wall -Wparenthesis -Wno-unused_variable -Wno-sign-compare")

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

include_directories("$PROJECT_SOURCE_DIR}")

find_package(Threads)
add_executable(scheduler scheduler_driver.cpp)
target_link_libraries(scheduler ${CMAKE_THREAD_LIBS_INIT})

