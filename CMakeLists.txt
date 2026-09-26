cmake_minimum_required(VERSION 3.16)
project(ollama_gen)

include(CTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE Release)
   message(STATUS "No build type selected, defaulting to Release")
else()
    message(STATUS "Build type selected: ${CMAKE_BUILD_TYPE}")
endif()


set(LIBRARY_NAME ollama_gen)
set(SOURCES mx2-ollama.cpp)
set(HEADERS mx2-ollama.hpp)

find_package(PkgConfig REQUIRED)
pkg_check_modules(CURL REQUIRED IMPORTED_TARGET libcurl)
pkg_check_modules(JSONCPP REQUIRED IMPORTED_TARGET jsoncpp)

message(STATUS "Found CURL: ${CURL_FOUND}")
message(STATUS "CURL include dirs: ${CURL_INCLUDE_DIRS}")
message(STATUS "CURL libraries: ${CURL_LIBRARIES}")
message(STATUS "Found jsoncpp: ${JSONCPP_VERSION}")

add_library(${LIBRARY_NAME} STATIC ${SOURCES} ${HEADERS})
set_target_properties(${LIBRARY_NAME} PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

target_include_directories(${LIBRARY_NAME} PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(${LIBRARY_NAME}
    PRIVATE
        PkgConfig::CURL
        PkgConfig::JSONCPP
)

if(BUILD_TESTING)
    add_executable(ollama_gen_json_test tests/json_stream_test.cpp)
    target_link_libraries(ollama_gen_json_test PRIVATE ${LIBRARY_NAME})
    add_test(NAME ollama_gen_json_stream COMMAND ollama_gen_json_test)
endif()

install(TARGETS ${LIBRARY_NAME} 
    EXPORT ollama_gen-targets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)
install(FILES ${HEADERS} DESTINATION include)

install(EXPORT ollama_gen-targets
    FILE ollama_gen-targets.cmake
    DESTINATION lib/cmake/ollama_gen
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/ollama_gen-config-version.cmake"
    VERSION 1.0.0
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/ollama_gen-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ollama_gen-config.cmake"
    INSTALL_DESTINATION lib/cmake/ollama_gen
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/ollama_gen-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/ollama_gen-config-version.cmake"
    DESTINATION lib/cmake/ollama_gen
)
