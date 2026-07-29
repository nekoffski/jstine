add_executable(jstine_unit_tests
    # core
    "core/test_Error.cpp"
    "core/test_FileSystem.cpp"
    "core/test_Functional.cpp"
    "core/test_Log.cpp"
    "core/test_OS.cpp"
    "core/test_Scope.cpp"
    "core/test_String.cpp"
    # runtime
    "runtime/test_ThreadSafeQueue.cpp"
    # adapters
    "adapters/asio/test_AsioStorageProxy.cpp"
    # storage
    "storage/api/test_StorageProxy.cpp"
    # protocol
    "proto/test_Message.cpp"
    "proto/test_MessageCodec.cpp"
    "proto/test_Protocol.cpp"
    "proto/test_JFP.cpp"
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
