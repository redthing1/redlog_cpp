# Examples CMakeLists.txt

# Basic usage example
add_executable(basic_example basic_example.cpp)
target_link_libraries(basic_example PRIVATE redlog::redlog)

# Advanced features example
add_executable(advanced_example advanced_example.cpp)
target_link_libraries(advanced_example PRIVATE redlog::redlog)

# Performance example
add_executable(performance_example performance_example.cpp)
target_link_libraries(performance_example PRIVATE redlog::redlog)

# Themes and formatting showcase
add_executable(themes_and_formatting themes_and_formatting.cpp)
target_link_libraries(themes_and_formatting PRIVATE redlog::redlog)