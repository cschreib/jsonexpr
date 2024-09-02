cmake_minimum_required(VERSION 3.18)

project(jsonexpr LANGUAGES CXX VERSION 1.0.0)

# Figure out git hash, if any
execute_process(
    COMMAND git log -1 --format=%h
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    RESULT_VARIABLE GIT_COMMAND_SUCCESS
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)

if (GIT_COMMAND_SUCCESS EQUAL 0)
    set(JSONEXPR_FULL_VERSION "${PROJECT_VERSION}.${GIT_COMMIT_HASH}")
else()
    set(JSONEXPR_FULL_VERSION "${PROJECT_VERSION}")
endif()

# Components (library is always built).
set(JSONEXPR_APP   ON   CACHE BOOL "Build command line application.")
set(JSONEXPR_TEST  OFF  CACHE BOOL "Build tests.")
set(JSONEXPR_FUZZ  OFF  CACHE BOOL "Build fuzzing application.")

# Dependencies.
set(JSONEXPR_USE_SYSTEM_JSON     OFF CACHE BOOL "Use a pre-installed version of nlohmann::json. Else, download.")
set(JSONEXPR_USE_SYSTEM_EXPECTED OFF CACHE BOOL "Use a pre-installed version of tl::expected. Else, download.")
set(JSONEXPR_USE_STD_EXPECTED    OFF CACHE BOOL "Use std::expected (requires C++23). Else, use tl::expected.")
set(JSONEXPR_USE_STD_FROM_CHARS  ON  CACHE BOOL "Use std::from_chars (requires C++17). Else, use streams.")

# Development options.
set(JSONEXPR_DEV OFF CACHE BOOL "Enable warnings in compilation.")

add_subdirectory(libjsonexpr)

if (JSONEXPR_APP)
    add_subdirectory(jsonexpr)
endif()

if (JSONEXPR_TEST)
    enable_testing()
    add_subdirectory(tests)
endif()

if (JSONEXPR_FUZZ)
    add_subdirectory(fuzzing/runner)
endif()
