cmake_minimum_required( VERSION 2.8 )

project( Utils )

# Enable testing
enable_testing()

# Make sure we find the header files with #include "utils/..."
include_directories( ${CMAKE_SOURCE_DIR}/.. )

add_subdirectory( tests )
