# SPDX-FileCopyrightText: 2014-2024 Technical University of Munich
# 
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)

project(utils)

add_library(utils INTERFACE)

set_property(TARGET utils PROPERTY CXX_STANDARD 17)

# Make sure we find the header files with #include "utils/..." (TODO: don't)
target_include_directories(utils INTERFACE ${CMAKE_SOURCE_DIR}/..)

option(TESTING "Build unit tests" OFF)

if (TESTING)
  # Enable testing
  enable_testing()

  add_subdirectory( tests )
endif()




