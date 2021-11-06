cmake_minimum_required(VERSION 3.12)
project(server)

file(GLOB SOURCES src/*.c)
add_executable(server ${SOURCES})

set(INCLUDES ${INCLUDES} include)
set(LIBS ${LIBS} pthread)

target_include_directories(server PUBLIC ${INCLUDES})
target_link_libraries(server PUBLIC ${LIBS})