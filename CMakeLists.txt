cmake_minimum_required(VERSION 3.5)
project(shell C)

find_package(X11 REQUIRED)
include_directories(${X11_INCLUDE_DIR})


set(CMAKE_C_STANDARD 23)

add_executable(shell main.c
        keysMapping.h
        thmall_shell.h
        terminal_shell_communication.h
        data_structs.h)
target_include_directories(shell PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(shell PRIVATE ${X11_LIBRARIES})
