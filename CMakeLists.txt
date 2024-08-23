cmake_minimum_required(VERSION 3.18)

project(jsonexpr LANGUAGES CXX VERSION 1.0.0)

set(JSONEXPR_APP   ON  CACHE BOOL "Build command line application.")
set(JSONEXPR_TEST  ON  CACHE BOOL "Build tests.")

add_subdirectory(libjsonexpr)

if (JSONEXPR_APP)
    add_subdirectory(jsonexpr)
endif()

if (JSONEXPR_TEST)
    enable_testing()
    add_subdirectory(tests)
endif()
