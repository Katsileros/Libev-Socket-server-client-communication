cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

PROJECT(socket_server)

add_library_libev_4_0_0()

SET(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable( socket_server socket_server.cpp solver.h)
add_executable( socket_client socket_client.cpp solver.h)
TARGET_LINK_LIBRARIES(socket_server pthread)
target_link_libraries(socket_server libev_4_0_0 )
target_link_libraries(socket_client libev_4_0_0 )
