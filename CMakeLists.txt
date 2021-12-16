cmake_minimum_required(VERSION 3.5)

project(prpc_c)

add_library(prpc_c STATIC
    src/prpc_c.c
)

target_include_directories(prpc_c
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)


# Add an executable with the above sources
add_executable(test
    src/test.c
)

# link the new hello_library target with the hello_binary target
target_link_libraries(test
    PRIVATE
        prpc_c
        ${CMAKE_CURRENT_LIST_DIR}/lib/libmsgpackc.a
)
