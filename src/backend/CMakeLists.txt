add_library(jstine_backend STATIC
    # -- core
    "jstine/core/Config.cpp"
    "jstine/core/Core.cpp"
    "jstine/core/Error.cpp"
    "jstine/core/FileSystem.cpp"
    "jstine/core/Log.cpp"
    "jstine/core/Os.cpp"
    "jstine/core/Profiler.cpp"
    "jstine/core/Random.cpp"
    "jstine/core/String.cpp"
    "jstine/core/TomlConfigReader.cpp"
    # -- render
    "jstine/render/RenderOrchestrator.cpp"
    "jstine/render/RenderRequest.cpp"
    "jstine/render/RenderSession.cpp"
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
