SET (CLI_SRCS 
    "main.cpp"
)

add_executable(jstine-cli ${CLI_SRCS})
target_link_libraries(jstine-cli jstine_backend)

if (BUILD_TESTING)
    add_test(
        NAME jstine-cli-starts-render-session
        COMMAND jstine-cli --config ${CMAKE_SOURCE_DIR}/conf/config-template.toml
    )
endif()
