cmake_minimum_required(VERSION 3.5)

project(prpc_c)

add_library(prpc_c SHARED
	src/prpc_c.c
)
set_target_properties(prpc_c PROPERTIES LINKER_LANGUAGE C)
target_link_libraries(prpc_c
    "-Wl,--whole-archive ${CMAKE_CURRENT_LIST_DIR}/lib/libmsgpackc.a -Wl,--no-whole-archive"
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
)
