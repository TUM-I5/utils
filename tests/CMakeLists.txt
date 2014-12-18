# Add cxxtest pathes
set( CXXTEST_PATH ${CMAKE_SOURCE_DIR}/submodules/cxxtest )
set( CMAKE_INCLUDE_PATH
    ${CMAKE_SOURCE_DIR}/submodules/cxxtest
    ${CMAKE_INCLUDE_PATH} )
set( CMAKE_MODULE_PATH
	${CMAKE_SOURCE_DIR}/submodules/cxxtest/build_tools/cmake
	${CMAKE_MODULE_PATH} )

# Get cxxtest
# TODO support installations without cxxtest
find_package( CxxTest REQUIRED )
include_directories( ${CXXTEST_INCLUDES} )

# Add tests
cxx_test( TestArgs ${CMAKE_CURRENT_SOURCE_DIR}/args.t.h )
cxx_test( TestEnv ${CMAKE_CURRENT_SOURCE_DIR}/env.t.h )
cxx_test( TestLogger ${CMAKE_CURRENT_SOURCE_DIR}/logger.t.h )
cxx_test( TestMathUtils ${CMAKE_CURRENT_SOURCE_DIR}/mathutils.t.h )
cxx_test( TestPath ${CMAKE_CURRENT_SOURCE_DIR}/path.t.h )
cxx_test( TestProgress ${CMAKE_CURRENT_SOURCE_DIR}/progress.t.h )
cxx_test( TestStringUtils ${CMAKE_CURRENT_SOURCE_DIR}/stringutils.t.h )
cxx_test( TestTimeUtils ${CMAKE_CURRENT_SOURCE_DIR}/timeutils.t.h )