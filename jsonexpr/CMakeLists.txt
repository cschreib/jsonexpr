cmake_minimum_required(VERSION 3.18)

project(jsonexpr LANGUAGES CXX)

add_executable(jsonexpr
    ${PROJECT_SOURCE_DIR}/src/main.cpp)

target_link_libraries(jsonexpr PUBLIC jsonexpr::libjsonexpr)

