cmake_minimum_required(VERSION 3.14)

project(IssueTracker VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(${PROJECT_NAME}_core STATIC)
target_include_directories(${PROJECT_NAME}_core PUBLIC include)
target_sources(${PROJECT_NAME}_core PRIVATE 
    src/parser.cpp
    src/cli/router.cpp
    src/cli/tasks.cpp
    src/cli/views.cpp
    src/cli/system.cpp
    src/engine.cpp
    src/storage.cpp
    src/config.cpp
)

add_executable(${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "issue-tracker")
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
target_sources(${PROJECT_NAME} PRIVATE src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_core)

find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
if(CLANG_TIDY_EXE)
    set(CMAKE_C_CLANG_TIDY ${CLANG_TIDY_EXE})
    set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXE})
endif()

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Werror)
endif()


# Testing
enable_testing()
add_subdirectory(tests)
