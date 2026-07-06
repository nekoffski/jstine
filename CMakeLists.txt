cmake_minimum_required(VERSION 3.22)
project(hyp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(tomlplusplus CONFIG REQUIRED)
find_package(stduuid REQUIRED)
find_package(asio REQUIRED)
find_package(CLI11)
find_package(GTest REQUIRED)

add_subdirectory(src)

enable_testing()
add_subdirectory(tests/unit)
