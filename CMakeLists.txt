cmake_minimum_required(VERSION 3.9)
project(project)

set(CMAKE_CXX_STANDARD 11)

set(SERVER_SOURCE_FILES server/isaserver.c)

set(CLIENT_SOURCE_FILES client/isaclient.c)

add_executable(client ${CLIENT_SOURCE_FILES})

add_executable(server ${SERVER_SOURCE_FILES})