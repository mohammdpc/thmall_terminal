cmake_minimum_required(VERSION 3.5)
project(shell C)

find_package(X11 REQUIRED)
include_directories(${X11_INCLUDE_DIR})


set(CMAKE_C_STANDARD 23)

add_executable(shell main.c)
target_include_directories(shell PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(shell PRIVATE ${X11_LIBRARIES})
