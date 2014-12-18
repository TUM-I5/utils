/**
 * @file
 *  This file is part of UTILS
 *
 * UTILS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * UTILS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with UTILS.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @copyright 2014 Technische Universitaet Muenchen
 * @author Sebastian Rettenberger <rettenbs@in.tum.de>
 */

#include <cxxtest/TestSuite.h>

#include "utils/path.h"

using namespace utils;

class TestPath : public CxxTest::TestSuite
{
public:
	void testToString()
	{
		TS_ASSERT_EQUALS(std::string(Path("foo/")), "foo");
	}

	void testBasename()
	{
		TS_ASSERT_EQUALS(Path("foo/bar").basename(), "bar");
		TS_ASSERT_EQUALS(Path("foo").basename(), "foo")
	}

	void testDirname()
	{
		TS_ASSERT_EQUALS(Path("foo/bar").dirname(), "foo");
		TS_ASSERT_EQUALS(Path("foo/foo/bar").dirname(), "foo/foo");
		TS_ASSERT_EQUALS(Path("foo").dirname(), "");
	}

	void testDir()
	{
		TS_ASSERT_EQUALS(std::string(Path("foo/bar").dir()), "foo");
	}

	void testOperatorPlus()
	{
		TS_ASSERT_EQUALS(std::string(Path("foo")+Path("bar")), "foo/bar");
		TS_ASSERT_EQUALS(std::string(Path("foo/")+Path("bar")), "foo/bar");
		TS_ASSERT_EQUALS(std::string(Path("foo")+Path("")), "foo");
	}
};
