cmake_minimum_required(VERSION 3.10)
project(chat_unix)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(ollama_gen REQUIRED)
add_executable(chat_unix chat.cpp)

set(OLLAMA_GEN_LIBRARIES ollama_gen)
set(OLLAMA_GEN_INCLUDE_DIRS ${OLLAMA_GEN_INCLUDE_DIRS})

target_link_libraries(chat_unix ${OLLAMA_GEN_LIBRARIES})
target_include_directories(chat_unix PRIVATE ${OLLAMA_GEN_INCLUDE_DIRS})
target_compile_options(chat_unix PRIVATE ${OLLAMA_GEN_CFLAGS_OTHER})