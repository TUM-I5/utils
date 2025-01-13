// SPDX-FileCopyrightText: 2014-2024 Technical University of Munich
//
// SPDX-License-Identifier: BSD-3-Clause
//
// SPDX-FileContributor: Sebastian Rettenberger

#ifndef UTILS_TESTS_STRINGUTILS_T_H_
#define UTILS_TESTS_STRINGUTILS_T_H_

#include "utils/stringutils.h"

using namespace utils;

class TestStringUtils : public CxxTest::TestSuite {
  public:
  static void testStartsWith() {
    TS_ASSERT(StringUtils::startsWith("abcde", "abc"));
    TS_ASSERT(!StringUtils::startsWith("abcde", "abd"));
  }

  static void testEndsWith() {
    TS_ASSERT(StringUtils::endsWith("abcde", "cde"));
    TS_ASSERT(!StringUtils::endsWith("abcde", "abc"));
  }

  static void testParse() {
    // Normal parser
    // TODO more tests
    TS_ASSERT_EQUALS(StringUtils::parse<int>("-1"), -1);

    // Advanced parser
    TS_ASSERT(StringUtils::parse<bool>("on"));
    TS_ASSERT(StringUtils::parse<bool>("yes"));
    TS_ASSERT(StringUtils::parse<bool>("on"));
    TS_ASSERT(!StringUtils::parse<bool>("off"));
    TS_ASSERT(!StringUtils::parse<bool>("abc"));
  }

  static void testParseArray() {
    // TODO more tests
    std::vector<int> result = StringUtils::parseArray<int>("1:2:3");

    TS_ASSERT_EQUALS(result[0], 1);
    TS_ASSERT_EQUALS(result[1], 2);
    TS_ASSERT_EQUALS(result[2], 3);
  }
};
#endif // UTILS_TESTS_STRINGUTILS_T_H_
