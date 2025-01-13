# SPDX-FileCopyrightText: 2014-2024 Technical University of Munich
# 
# SPDX-License-Identifier: BSD-3-Clause

find_package( CxxTest REQUIRED )
include_directories( ${CXXTEST_INCLUDE_DIR} )

function( cxx_test target source )
    get_filename_component(CPP_FILE_NAME ${source} NAME)
    string(REGEX REPLACE "h$|hpp$" "cpp" CPP_FILE_NAME ${CPP_FILE_NAME})
    set(CPP_FULL_NAME "${CMAKE_CURRENT_BINARY_DIR}/${CPP_FILE_NAME}")
    CXXTEST_ADD_TEST( ${target} "${CPP_FULL_NAME}" ${source} )
    target_link_libraries( ${target} PRIVATE utils )
    set_property(TARGET ${target} PROPERTY CXX_STANDARD 17)
endfunction( cxx_test )

# Add tests
cxx_test( TestArgs ${CMAKE_CURRENT_SOURCE_DIR}/args.t.h )
cxx_test( TestEnv ${CMAKE_CURRENT_SOURCE_DIR}/env.t.h )
cxx_test( TestLogger ${CMAKE_CURRENT_SOURCE_DIR}/logger.t.h )
cxx_test( TestMathUtils ${CMAKE_CURRENT_SOURCE_DIR}/mathutils.t.h )
cxx_test( TestPath ${CMAKE_CURRENT_SOURCE_DIR}/path.t.h )
cxx_test( TestProgress ${CMAKE_CURRENT_SOURCE_DIR}/progress.t.h )
cxx_test( TestStringUtils ${CMAKE_CURRENT_SOURCE_DIR}/stringutils.t.h )
cxx_test( TestTimeUtils ${CMAKE_CURRENT_SOURCE_DIR}/timeutils.t.h )
