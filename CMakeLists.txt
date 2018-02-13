cmake_minimum_required(VERSION 2.8.12)

project(dither)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

file(GLOB HDRS "*.h")
file(GLOB SRCS "*.cpp")

add_executable(${PROJECT_NAME} ${HDRS} ${SRCS})
