add_library(jstined_lib STATIC
    "Config.cpp"
)

target_link_libraries(jstined_lib PUBLIC jstine_internal)
target_include_directories(jstined_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(justined "main.cpp")
target_link_libraries(justined PRIVATE jstined_lib)