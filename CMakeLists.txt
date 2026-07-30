cmake_minimum_required(VERSION 3.22)
project(hyp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(JSTINE_ENABLE_PROFILING OFF)
option(
    JSTINE_ENABLE_COVERAGE
    "Instrument project targets for unit-test coverage"
    OFF
)

if (JSTINE_ENABLE_COVERAGE)
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(
            -O0
            -g
            -fprofile-instr-generate
            -fcoverage-mapping
        )
        add_link_options(-fprofile-instr-generate)
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_compile_options(-O0 -g --coverage)
        add_link_options(--coverage)
    else()
        message(
            FATAL_ERROR
            "Coverage requires Clang, AppleClang, or GCC; found "
            "${CMAKE_CXX_COMPILER_ID}"
        )
    endif()

    file(
        GENERATE
        OUTPUT "${CMAKE_BINARY_DIR}/jstine-coverage-toolchain.txt"
        CONTENT "${CMAKE_CXX_COMPILER_ID}\n${CMAKE_CXX_COMPILER_VERSION}\n"
    )
endif()

find_package(spdlog REQUIRED)
find_package(tomlplusplus CONFIG REQUIRED)
find_package(stduuid REQUIRED)
find_package(asio REQUIRED)
find_package(CLI11)
find_package(GTest REQUIRED)

add_subdirectory(src)

enable_testing()
add_subdirectory(tests/unit)
