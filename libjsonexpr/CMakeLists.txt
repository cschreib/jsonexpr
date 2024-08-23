cmake_minimum_required(VERSION 3.18)

project(libjsonexpr LANGUAGES CXX VERSION 0.1.0)

include(FetchContent)

FetchContent_Declare(json
                     GIT_REPOSITORY https://github.com/nlohmann/json.git
                     GIT_TAG        v3.11.3)
FetchContent_MakeAvailable(json)

add_library(libjsonexpr
    ${PROJECT_SOURCE_DIR}/src/ast.cpp
    ${PROJECT_SOURCE_DIR}/src/parse.cpp
    ${PROJECT_SOURCE_DIR}/src/eval.cpp
    ${PROJECT_SOURCE_DIR}/src/jsonexpr.cpp)

target_compile_features(libjsonexpr PUBLIC cxx_std_20)
target_include_directories(libjsonexpr PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>)

target_link_libraries(libjsonexpr PUBLIC
    nlohmann_json::nlohmann_json)

add_library(jsonexpr::libjsonexpr ALIAS libjsonexpr)
