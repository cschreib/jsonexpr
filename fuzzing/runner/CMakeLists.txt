cmake_minimum_required(VERSION 3.18)

project(jsonexpr-fuzzing-runner LANGUAGES CXX)

add_executable(jsonexpr-fuzzing-runner
    ${PROJECT_SOURCE_DIR}/src/main.cpp)

target_link_libraries(jsonexpr-fuzzing-runner PUBLIC jsonexpr::libjsonexpr)

