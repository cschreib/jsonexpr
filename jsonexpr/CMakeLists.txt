cmake_minimum_required(VERSION 3.18)

project(jsonexpr LANGUAGES CXX VERSION 0.1.0)

add_executable(jsonexpr
    ${PROJECT_SOURCE_DIR}/src/main.cpp)

target_link_libraries(jsonexpr PUBLIC jsonexpr::libjsonexpr)

