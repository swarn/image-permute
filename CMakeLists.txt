cmake_minimum_required(VERSION 3.15)

project(image-permute CXX)

# use modern C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# If no build type was specified, and we're not in a (typically IDE)
# environment with multiple build types, set the build type.
set(default_build_type "Release")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS
        "Setting build type to '${default_build_type}' as none was specified.")
    set(CMAKE_BUILD_TYPE
        "${default_build_type}" CACHE STRING "Build type" FORCE)
    set_property(CACHE
        CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Compiler warnings. This is strange-looking, but in modern cmake, "library"
# just means "a node in the DAG of project dependencies and settings."
add_library(project_warnings INTERFACE)
include(cmake/CompilerWarnings.cmake)
set_project_warnings(project_warnings)

# sanitizer options if supported by compiler
include(cmake/StaticAnalyzers.cmake)

# Create a list of every compile command used for each file, useful for
# external linters, syntax checkers, etc.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# put the executables in bin
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)

# Third-party code
add_subdirectory(external)

add_subdirectory(src)

