include_directories(include)

file(GLOB SOURCES "src/*.cpp")

add_executable(server ${SOURCES})
target_include_directories(client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(server PRIVATE common)
