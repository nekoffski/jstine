add_executable(jstine_unit_tests
    "test_core_Error.cpp"
    "test_core_FileSystem.cpp"
    "test_core_Functional.cpp"
    "test_core_Log.cpp"
    "test_core_OS.cpp"
    "test_core_Scope.cpp"
    "test_core_String.cpp"
    "test_proto_Message.cpp"
    "test_proto_MessageCodec.cpp"
    "test_proto_Protocol.cpp"
    "test_proto_JFP.cpp"
    "test_storage_StorageManager.cpp"
)

target_link_libraries(jstine_unit_tests
    PRIVATE
        jstined_lib
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(jstine_unit_tests)
