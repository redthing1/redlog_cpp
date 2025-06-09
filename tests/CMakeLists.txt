# Tests CMakeLists.txt

# Enable testing
enable_testing()

# Simple test runner - no external dependencies
add_executable(redlog_tests test_main.cpp)
target_link_libraries(redlog_tests PRIVATE redlog::redlog)

# Add test
add_test(NAME redlog_basic_tests COMMAND redlog_tests)