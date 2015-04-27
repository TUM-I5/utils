/**
 * @file
 *  This file is part of UTILS
 *
 * @author Sebastian Rettenberger <sebastian.rettenberger@tum.de>
 *
 * @copyright Copyright (c) 2014-2015, Technische Universitaet Muenchen.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright notice
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 *  3. Neither the name of the copyright holder nor the names of its
 *     contributors may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#include <cxxtest/TestSuite.h>

#include <cstdlib>

#include "utils/env.h"

using namespace utils;

class TestEnv : public CxxTest::TestSuite
{
public:
	void testGet()
	{
		TS_ASSERT_EQUALS(setenv("UTILS_INT", "42", 1), 0);
		TS_ASSERT_EQUALS(Env::get<int>("UTILS_INT", 0), 42);
		TS_ASSERT_EQUALS(Env::get<int>("UTILS_INT2", 3), 3);

		TS_ASSERT_EQUALS(setenv("UTILS_BOOL", "1", 1), 0);
		TS_ASSERT_EQUALS(Env::get<bool>("UTILS_BOOL", false), true);
		TS_ASSERT_EQUALS(setenv("UTILS_BOOL", "0", 1), 0);
		TS_ASSERT_EQUALS(Env::get<bool>("UTILS_BOOL", false), false);
	}
};
