cmake_minimum_required(VERSION 3.15)
project(mouse VERSION 1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(common)
add_subdirectory(test)

enable_testing()
