cmake_minimum_required(VERSION 3.16)
project(ollama_gen_example)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(ollama_gen REQUIRED PATHS ${CMAKE_CURRENT_SOURCE_DIR}/../build)

add_executable(example code.cpp)

target_link_libraries(example ollama_gen)

if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE Release)
   message(STATUS "No build type selected, defaulting to Release")
else()
    message(STATUS "Build type selected: ${CMAKE_BUILD_TYPE}")
endif()