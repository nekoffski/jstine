add_executable(jstine_unit_tests
    # core
    "core/ErrorTests.cpp"
    "core/FileSystemTests.cpp"
    "core/FunctionalTests.cpp"
    "core/LogTests.cpp"
    "core/OSTests.cpp"
    "core/ScopeTests.cpp"
    "core/StringTests.cpp"
    # runtime
    "runtime/ThreadSafeQueueTests.cpp"
    # adapters
    "adapters/asio/AsioStorageProxyTests.cpp"
    # storage
    "storage/DatabaseTests.cpp"
    "storage/ExpirationRegistryTests.cpp"
    "storage/keyspace/StdKeyspaceTests.cpp"
    "storage/kv/KeyTests.cpp"
    "storage/kv/ValueTests.cpp"
    "storage/api/StorageProxyTests.cpp"
    # protocol
    "proto/MessageTests.cpp"
    "proto/MessageCodecTests.cpp"
    "proto/ProtocolTests.cpp"
    "proto/JFPTests.cpp"
)

target_include_directories(jstine_unit_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(jstine_unit_tests
    PRIVATE
        jstined_lib
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(jstine_unit_tests)
