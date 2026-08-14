SET (CLI_SRCS 
    "main.cpp"
)

add_executable(jstine-cli ${CLI_SRCS})
target_link_libraries(jstine-cli jstine_backend)