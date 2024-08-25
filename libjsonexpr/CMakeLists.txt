cmake_minimum_required(VERSION 3.18)

project(libjsonexpr LANGUAGES CXX)

include(FetchContent)

if (NOT JSONEXPR_USE_SYSTEM_JSON)
    FetchContent_Declare(json
                         GIT_REPOSITORY https://github.com/nlohmann/json.git
                         GIT_TAG        v3.11.3)
    FetchContent_MakeAvailable(json)
endif()
if (NOT JSONEXPR_USE_STD_EXPECTED AND NOT JSONEXPR_USE_SYSTEM_EXPECTED)
    set(EXPECTED_BUILD_TESTS OFF)
    set(EXPECTED_BUILD_PACKAGE_DEB OFF)
    FetchContent_Declare(expected
                         GIT_REPOSITORY https://github.com/TartanLlama/expected.git
                         GIT_TAG        v1.1.0)
    FetchContent_MakeAvailable(expected)
endif()

if (DEFINED BUILD_SHARED_LIBS)
    set(JSONEXPR_SHARED_LIBRARY ${BUILD_SHARED_LIBS})
endif()

configure_file("${PROJECT_SOURCE_DIR}/include/jsonexpr/config.hpp.config"
               "${PROJECT_BINARY_DIR}/jsonexpr/config.hpp")

add_library(libjsonexpr
    ${PROJECT_SOURCE_DIR}/src/ast.cpp
    ${PROJECT_SOURCE_DIR}/src/base.cpp
    ${PROJECT_SOURCE_DIR}/src/eval.cpp
    ${PROJECT_SOURCE_DIR}/src/functions.cpp
    ${PROJECT_SOURCE_DIR}/src/parse.cpp
    ${PROJECT_SOURCE_DIR}/src/jsonexpr.cpp)

if (JSONEXPR_DEV)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(libjsonexpr PRIVATE -Wall)
        target_compile_options(libjsonexpr PRIVATE -Wextra)
        target_compile_options(libjsonexpr PRIVATE -Werror)
        target_compile_options(libjsonexpr PRIVATE -pedantic)
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        target_compile_options(libjsonexpr PRIVATE -Wall)
        target_compile_options(libjsonexpr PRIVATE -Wextra)
        target_compile_options(libjsonexpr PRIVATE -Werror)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(libjsonexpr PRIVATE /W4)
        target_compile_options(libjsonexpr PRIVATE /WX)
        target_compile_options(libjsonexpr PRIVATE /EHs)
    endif()
endif()

if (BUILD_SHARED_LIBS)
    target_compile_definitions(libjsonexpr PRIVATE JSONEXPR_EXPORTS)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR MINGW)
        # Nothing to do; default is already to hide symbols unless exported.
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
        CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR
        CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        # Set default visibility to "hidden" so only exported symbols are visible.
        target_compile_options(libjsonexpr PRIVATE -fvisibility=hidden)
        target_compile_options(libjsonexpr PRIVATE -fvisibility-inlines-hidden)
    endif()
endif()

target_compile_features(libjsonexpr PUBLIC cxx_std_20)
target_include_directories(libjsonexpr PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>)

target_link_libraries(libjsonexpr PUBLIC nlohmann_json::nlohmann_json)

if (NOT JSONEXPR_USE_STD_EXPECTED)
    target_link_libraries(libjsonexpr PUBLIC tl::expected)
endif()

add_library(jsonexpr::libjsonexpr ALIAS libjsonexpr)
