cmake_minimum_required(VERSION 3.12)
project(client)

file(GLOB SOURCES src/*.c)
add_executable(client ${SOURCES})

set(INCLUDES ${INCLUDES} include)
set(LIBS ${LIBS} pthread)

target_include_directories(client PUBLIC ${INCLUDES})
target_link_libraries(client PUBLIC ${LIBS})