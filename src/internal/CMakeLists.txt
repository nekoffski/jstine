add_library(jstine_internal STATIC
    # -- core
    "core/Core.cpp"
    "core/Error.cpp"
    # "core/FileSystem.cpp"
    "core/Random.cpp"
    "core/Log.cpp"
    "core/Profiler.cpp"
    "core/OS.cpp"
    "core/String.cpp"
    # -- proto
)

target_include_directories(jstine_internal
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(jstine_internal
    PUBLIC
        fmt::fmt
        spdlog::spdlog
        stduuid::stduuid
)

set_target_properties(jstine_internal PROPERTIES POSITION_INDEPENDENT_CODE ON)
