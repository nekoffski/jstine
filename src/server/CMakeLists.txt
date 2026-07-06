add_library(jstined_lib STATIC
    "Server.cpp"
    # -- adapters
    "adapters/TomlConfigReader.cpp"
    "adapters/asio/AsioServer.cpp"
    "adapters/asio/AsioSession.cpp"
    "adapters/platform/Platform.cpp"
    "adapters/platform/unix/Signal.cpp"
    # -- api
    "api/MessageHandler.cpp"
    "api/Message.cpp"
    # -- core
    "core/Config.cpp"
    "core/Core.cpp"
    "core/Error.cpp"
    "core/FileSystem.cpp"
    "core/Log.cpp"
    "core/OS.cpp"
    "core/Profiler.cpp"
    "core/String.cpp"
    # -- runtime
    "runtime/Thread.cpp"
    "runtime/Signal.cpp"
    # -- proto
    "proto/MessageCodec.cpp"
    "proto/Protocol.cpp"
    "proto/impl/RESP.cpp"
    "proto/impl/JFP.cpp"
    # -- storage
    "storage/StorageManager.cpp"
    "storage/std/StdStorageManager.cpp"

)

target_include_directories(jstined_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(jstined_lib
    PUBLIC
        fmt::fmt
        spdlog::spdlog
        stduuid::stduuid
        CLI11::CLI11
        asio::asio
        tomlplusplus::tomlplusplus
)

add_executable(jstined "main.cpp")
target_link_libraries(jstined PRIVATE jstined_lib)