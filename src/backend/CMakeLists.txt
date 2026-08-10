add_library(jstine_backend STATIC
    # -- core
    "core/Config.cpp"
    "core/Core.cpp"
    "core/Error.cpp"
    "core/FileSystem.cpp"
    "core/Log.cpp"
    "core/Os.cpp"
    "core/Profiler.cpp"
    "core/Random.cpp"
    "core/String.cpp"
    "core/TomlConfigReader.cpp"
)

target_include_directories(jstine_backend
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(jstine_backend
    PUBLIC
        fmt::fmt
        spdlog::spdlog
        stduuid::stduuid
        CLI11::CLI11
        tomlplusplus::tomlplusplus
)

if (JSTINE_ENABLE_PROFILING)
    target_compile_definitions(jstine_backend PRIVATE JSTINE_ENABLE_PROFILING=1)
endif()
