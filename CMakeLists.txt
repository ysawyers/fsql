cmake_minimum_required(VERSION 3.29.3)
project(filecomposer)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME} src/main.cpp src/lexer.cpp src/parser.cpp src/runtime.cpp)
