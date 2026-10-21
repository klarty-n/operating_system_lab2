cmake_minimum_required(VERSION 3.10)

project(2nd_lab, LANGUAGES C)

set(CMAKE_C_STANDARD 99)
add_compile_options(-Wall -Wextra)

add_executable(parent parent.c)
add_executable(child child.c)
