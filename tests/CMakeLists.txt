add_executable(test_suite test_parser.cpp)
target_link_libraries(test_suite PRIVATE ${PROJECT_NAME}_core)

include(FetchContent)
FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)
target_link_libraries(test_suite PRIVATE gtest_main)
include(GoogleTest)
gtest_discover_tests(test_suite)
