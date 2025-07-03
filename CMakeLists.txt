cmake_minimum_required(VERSION 3.14)

project(redlog 
    VERSION 1.0.0
    DESCRIPTION "Modern C++ header-only logging library"
    LANGUAGES CXX)

# set C++17 as the minimum standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# add the header-only library
add_library(redlog INTERFACE)
add_library(redlog::redlog ALIAS redlog)

# specify include directories
target_include_directories(redlog 
    INTERFACE 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# require C++17
target_compile_features(redlog INTERFACE cxx_std_17)

# platform-specific libraries and definitions
if(WIN32)
    # windows doesn't need additional libraries for _isatty
    # add windows compile definitions to prevent macro conflicts
    target_compile_definitions(redlog INTERFACE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        _CRT_SECURE_NO_WARNINGS
    )
else()
    # unix-like systems need unistd.h (standard)
endif()

# add compiler warnings for development
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    target_compile_options(redlog INTERFACE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
    )
endif()

# option to build examples and tests
option(REDLOG_BUILD_EXAMPLES "Build redlog examples" ON)
option(REDLOG_BUILD_TESTS "Build redlog tests" ON)

# build examples if requested
if(REDLOG_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# build tests if requested  
if(REDLOG_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# installation configuration
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# install the header
install(FILES include/redlog.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# install the library target
install(TARGETS redlog
    EXPORT redlogTargets
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# create and install package config
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

# export targets for build tree usage
export(EXPORT redlogTargets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/redlogTargets.cmake"
    NAMESPACE redlog::
)

# register package in user package registry
export(PACKAGE redlog)

# optional clang-format integration
find_program(CLANG_FORMAT_EXE NAMES "clang-format")
if(CLANG_FORMAT_EXE)
    # collect all source files for formatting
    file(GLOB_RECURSE ALL_SOURCE_FILES 
        ${CMAKE_CURRENT_SOURCE_DIR}/include/redlog.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp
    )
    
    # add format target
    add_custom_target(redlog-format
        COMMAND ${CLANG_FORMAT_EXE} -i ${ALL_SOURCE_FILES}
        COMMENT "formatting redlog code with clang-format"
        VERBATIM
    )
    
    # add format-check target
    add_custom_target(redlog-format-check
        COMMAND ${CLANG_FORMAT_EXE} --dry-run --Werror ${ALL_SOURCE_FILES}
        COMMENT "checking redlog code formatting with clang-format"
        VERBATIM
    )
    
    # message(STATUS "clang-format found: ${CLANG_FORMAT_EXE}")
    # message(STATUS "  - Run 'make redlog-format' to format redlog code")
    # message(STATUS "  - Run 'make redlog-format-check' to check redlog formatting")
else()
    message(STATUS "clang-format not found - formatting targets not available")
endif()