cmake_minimum_required(VERSION 3.18)

project(jsonexpr_tests LANGUAGES CXX)

include(FetchContent)

set(SNITCH_DEFINE_MAIN OFF)
FetchContent_Declare(snitch
                     GIT_REPOSITORY https://github.com/cschreib/snitch.git
                     GIT_TAG        v1.2.5)
FetchContent_MakeAvailable(snitch)

add_executable(jsonexpr_tests
    src/array.cpp
    src/base.cpp
    src/bool.cpp
    src/functions.cpp
    src/if.cpp
    src/main.cpp
    src/misc.cpp
    src/number.cpp
    src/object.cpp
    src/string.cpp
    src/variable.cpp)
target_link_libraries(jsonexpr_tests PRIVATE jsonexpr::libjsonexpr)
target_link_libraries(jsonexpr_tests PRIVATE snitch::snitch)

add_custom_target(jsonexpr_tests_run
    COMMAND jsonexpr_tests
    WORKING_DIRECTORY ${RUNTIME_TEST_WORKING_DIRECTORY}
    SOURCES ${RUNTIME_TEST_FILES})
set_target_properties(jsonexpr_tests_run PROPERTIES EXCLUDE_FROM_ALL True)

if (BUILD_SHARED_LIBS AND WIN32)
    add_custom_command(TARGET jsonexpr_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:jsonexpr::libjsonexpr> $<TARGET_FILE_DIR:jsonexpr_tests>)
    add_custom_command(TARGET jsonexpr_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:snitch::snitch> $<TARGET_FILE_DIR:jsonexpr_tests>)
endif()
