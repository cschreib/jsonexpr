cmake_minimum_required(VERSION 3.18)

project(jsonexpr_tests LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(snitch
                     GIT_REPOSITORY https://github.com/cschreib/snitch.git
                     GIT_TAG        v1.2.5)
FetchContent_MakeAvailable(snitch)

add_executable(jsonexpr_tests src/main.cpp)
target_link_libraries(jsonexpr_tests PRIVATE jsonexpr::libjsonexpr)
target_link_libraries(jsonexpr_tests PRIVATE snitch::snitch)

add_custom_target(jsonexpr_tests_run
    COMMAND jsonexpr_tests
    WORKING_DIRECTORY ${RUNTIME_TEST_WORKING_DIRECTORY}
    SOURCES ${RUNTIME_TEST_FILES})
set_target_properties(jsonexpr_tests_run PROPERTIES EXCLUDE_FROM_ALL True)
