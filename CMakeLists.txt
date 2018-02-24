cmake_minimum_required(VERSION 3.7)
project(Reactant)

find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

set(CMAKE_C_STANDARD 11)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -Wextra")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(SOURCE_FILES main.c lib/tcp.c lib/tcp.h lib/ui.c lib/ui.h)
add_executable(Reactant ${SOURCE_FILES})
target_link_libraries(Reactant ${CURSES_LIBRARIES})
