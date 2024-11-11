// SPDX-FileCopyrightText: 2014-2024 Technical University of Munich
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef UTILS_TESTS_PATH_T_H_
#define UTILS_TESTS_PATH_T_H_

#include "utils/path.h"

using namespace utils;

class TestPath : public CxxTest::TestSuite {
  public:
  static void testToString() { TS_ASSERT_EQUALS(std::string(Path("foo/")), "foo"); }

  static void testBasename() {
    TS_ASSERT_EQUALS(Path("foo/bar").basename(), "bar");
    TS_ASSERT_EQUALS(Path("foo").basename(), "foo")
  }

  static void testDirname() {
    TS_ASSERT_EQUALS(Path("foo/bar").dirname(), "foo");
    TS_ASSERT_EQUALS(Path("foo/foo/bar").dirname(), "foo/foo");
    TS_ASSERT_EQUALS(Path("foo").dirname(), "");
  }

  static void testDir() { TS_ASSERT_EQUALS(std::string(Path("foo/bar").dir()), "foo"); }

  static void testExists() {
    TS_ASSERT(Path("/dev/null").exists());
    TS_ASSERT(!Path("/dev/asdfasdf").exists());
  }

  static void testOperatorPlus() {
    TS_ASSERT_EQUALS(std::string(Path("foo") + Path("bar")), "foo/bar");
    TS_ASSERT_EQUALS(std::string(Path("foo/") + Path("bar")), "foo/bar");
    TS_ASSERT_EQUALS(std::string(Path("foo") + Path("")), "foo");
  }
};
#endif // UTILS_TESTS_PATH_T_H_
