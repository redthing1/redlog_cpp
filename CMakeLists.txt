cmake_minimum_required(VERSION 3.14)

project(redlog 
    VERSION 1.0.0
    DESCRIPTION "Modern C++ header-only logging library"
    LANGUAGES CXX)

# Set C++17 as the minimum standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add the header-only library
add_library(redlog INTERFACE)
add_library(redlog::redlog ALIAS redlog)

# Specify include directories
target_include_directories(redlog 
    INTERFACE 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Require C++17
target_compile_features(redlog INTERFACE cxx_std_17)

# Platform-specific libraries for TTY detection
if(WIN32)
    # Windows doesn't need additional libraries for _isatty
else()
    # Unix-like systems need unistd.h (standard)
endif()

# Add compiler warnings for development
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    target_compile_options(redlog INTERFACE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
    )
endif()

# Option to build examples and tests
option(REDLOG_BUILD_EXAMPLES "Build redlog examples" ON)
option(REDLOG_BUILD_TESTS "Build redlog tests" ON)

# Build examples if requested
if(REDLOG_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Build tests if requested  
if(REDLOG_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Installation configuration
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Install the header
install(FILES include/redlog/redlog.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/redlog
)

# Install the library target
install(TARGETS redlog
    EXPORT redlogTargets
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Create and install package config
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/redlogConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/redlogConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/redlog
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/redlogConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/redlogConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/redlogConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/redlog
)

install(EXPORT redlogTargets
    FILE redlogTargets.cmake
    NAMESPACE redlog::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/redlog
)

# Export targets for build tree usage
export(EXPORT redlogTargets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/redlogTargets.cmake"
    NAMESPACE redlog::
)

# Register package in user package registry
export(PACKAGE redlog)

# Optional clang-format integration
find_program(CLANG_FORMAT_EXE NAMES "clang-format")
if(CLANG_FORMAT_EXE)
    # Collect all source files for formatting
    file(GLOB_RECURSE ALL_SOURCE_FILES 
        ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp
    )
    
    # Add format target
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXE} -i ${ALL_SOURCE_FILES}
        COMMENT "Formatting code with clang-format"
        VERBATIM
    )
    
    # Add format-check target (doesn't modify files)
    add_custom_target(format-check
        COMMAND ${CLANG_FORMAT_EXE} --dry-run --Werror ${ALL_SOURCE_FILES}
        COMMENT "Checking code formatting with clang-format"
        VERBATIM
    )
    
    message(STATUS "clang-format found: ${CLANG_FORMAT_EXE}")
    message(STATUS "  - Run 'make format' to format code")
    message(STATUS "  - Run 'make format-check' to check formatting")
else()
    message(STATUS "clang-format not found - formatting targets not available")
endif()