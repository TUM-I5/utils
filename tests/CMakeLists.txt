# Copyright (c) 2014-2024, Technische Universitaet Muenchen
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 
# 2. Redistributions in binary form must reproduce the above copyright notice
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived from this
#    software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

find_package( CxxTest REQUIRED )
include_directories( ${CXXTEST_INCLUDE_DIR} )

function( cxx_test target source )
    get_filename_component(CPP_FILE_NAME ${source} NAME)
    string(REGEX REPLACE "h$|hpp$" "cpp" CPP_FILE_NAME ${CPP_FILE_NAME})
    set(CPP_FULL_NAME "${CMAKE_CURRENT_BINARY_DIR}/${CPP_FILE_NAME}")
    CXXTEST_ADD_TEST( ${target} "${CPP_FULL_NAME}" ${source} )
    target_link_libraries( ${target} PRIVATE utils )
endfunction( cxx_test )

# Add tests
cxx_test( TestArgs ${CMAKE_CURRENT_SOURCE_DIR}/args.t.h )
cxx_test( TestArrayUtils ${CMAKE_CURRENT_SOURCE_DIR}/arrayutils.t.h )
cxx_test( TestCommon ${CMAKE_CURRENT_SOURCE_DIR}/common.t.h )
cxx_test( TestEnv ${CMAKE_CURRENT_SOURCE_DIR}/env.t.h )
cxx_test( TestLogger ${CMAKE_CURRENT_SOURCE_DIR}/logger.t.h )
cxx_test( TestMathUtils ${CMAKE_CURRENT_SOURCE_DIR}/mathutils.t.h )
cxx_test( TestPath ${CMAKE_CURRENT_SOURCE_DIR}/path.t.h )
cxx_test( TestProgress ${CMAKE_CURRENT_SOURCE_DIR}/progress.t.h )
cxx_test( TestStringUtils ${CMAKE_CURRENT_SOURCE_DIR}/stringutils.t.h )
cxx_test( TestTimeUtils ${CMAKE_CURRENT_SOURCE_DIR}/timeutils.t.h )