cmake_minimum_required(VERSION 3.18)

project(libjsonexpr LANGUAGES CXX)

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

set(LIBJSONEXPR_EXPORT_TARGETS "")
if (NOT JSONEXPR_USE_SYSTEM_JSON)
    FetchContent_Declare(json
                         URL https://github.com/nlohmann/json/archive/refs/tags/v3.11.3.zip)
    FetchContent_MakeAvailable(json)
    list(APPEND LIBJSONEXPR_EXPORT_TARGETS nlohmann_json)
else()
    find_package(nlohmann_json REQUIRED)
endif()

if (NOT JSONEXPR_USE_STD_EXPECTED AND NOT JSONEXPR_USE_SYSTEM_EXPECTED)
    set(EXPECTED_BUILD_TESTS OFF)
    set(EXPECTED_BUILD_PACKAGE_DEB OFF)
    FetchContent_Declare(expected
                         URL https://github.com/TartanLlama/expected/archive/refs/tags/v1.1.0.zip)
    FetchContent_MakeAvailable(expected)
    list(APPEND LIBJSONEXPR_EXPORT_TARGETS expected)
else()
    find_package(tl-expected REQUIRED)
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

set_target_properties(libjsonexpr PROPERTIES PREFIX "")
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
set_target_properties(libjsonexpr PROPERTIES EXPORT_NAME jsonexpr::libjsonexpr)

list(APPEND LIBJSONEXPR_EXPORT_TARGETS libjsonexpr)

set(LIBJSONEXPR_INCLUDES
    ${PROJECT_SOURCE_DIR}/include/jsonexpr/ast.hpp
    ${PROJECT_SOURCE_DIR}/include/jsonexpr/base.hpp
    ${PROJECT_SOURCE_DIR}/include/jsonexpr/eval.hpp
    ${PROJECT_SOURCE_DIR}/include/jsonexpr/expected.hpp
    ${PROJECT_SOURCE_DIR}/include/jsonexpr/functions.hpp
    ${PROJECT_SOURCE_DIR}/include/jsonexpr/jsonexpr.hpp
    ${PROJECT_SOURCE_DIR}/include/jsonexpr/parse.hpp
    ${PROJECT_BINARY_DIR}/jsonexpr/config.hpp)

install(
    FILES ${LIBJSONEXPR_INCLUDES}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/jsonexpr)

# Setup CMake config file
install(TARGETS ${LIBJSONEXPR_EXPORT_TARGETS} EXPORT libjsonexpr-targets)

install(EXPORT libjsonexpr-targets
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/jsonexpr
    NAMESPACE jsonexpr::
    COMPONENT Development)

export(EXPORT libjsonexpr-targets)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/jsonexpr-config.cmake.in"
    "${PROJECT_BINARY_DIR}/jsonexpr-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
    NO_SET_AND_CHECK_MACRO)

install(FILES
    "${PROJECT_BINARY_DIR}/jsonexpr-config.cmake"
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/jsonexpr COMPONENT Development)
