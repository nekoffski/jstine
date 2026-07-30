add_library(jstined_lib STATIC
    "Server.cpp"
    # -- adapters
    "adapters/TomlConfigReader.cpp"
    "adapters/asio/AsioServer.cpp"
    "adapters/asio/AsioSession.cpp"
    "adapters/asio/AsioMessageHandler.cpp"
    "adapters/asio/AsioStorageProxy.cpp"
    "adapters/platform/Platform.cpp"
    "adapters/platform/unix/Signal.cpp"
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
    "proto/Message.cpp"
    # -- mem
    "mem/Mallocator.cpp"
    # -- storage
    "storage/StorageEngine.cpp"
    "storage/ExpirationRegistry.cpp"
    "storage/Database.cpp"
    "storage/Reaper.cpp"
    "storage/keyspace/StdKeyspace.cpp"
    "storage/hash/FNVHash.cpp"
    "storage/kv/Key.cpp"
    "storage/kv/StrValue.cpp"
    "storage/kv/ValueWrapper.cpp"
    "storage/kv/EmptyValue.cpp"
    "storage/api/StorageProxy.cpp"
    "storage/api/StorageTransaction.cpp"
    "storage/api/StorageExecutor.cpp"
    "storage/api/StorageCommandQueue.cpp"
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

if (JSTINE_ENABLE_PROFILING)
    target_compile_definitions(jstined_lib PRIVATE JSTINE_ENABLE_PROFILING=1)
    target_compile_definitions(jstined PRIVATE JSTINE_ENABLE_PROFILING=1)
endif()
