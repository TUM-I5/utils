// SPDX-FileCopyrightText: 2014-2024 Technical University of Munich
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef UTILS_TESTS_MATHUTILS_T_H_
#define UTILS_TESTS_MATHUTILS_T_H_

#include "utils/mathutils.h"

using namespace utils;

class TestMathUtils : public CxxTest::TestSuite {
  public:
  static void testRoundUp() {
    TS_ASSERT_EQUALS(MathUtils::roundUp(6, 4), 8);
    TS_ASSERT_EQUALS(MathUtils::roundUp(3, 3), 3);
    TS_ASSERT_EQUALS(MathUtils::roundUp(12, 10), 20);
  }

  static void testGcd() { TS_ASSERT_EQUALS(MathUtils::gcd(6, 9), 3); }
};
#endif // UTILS_TESTS_MATHUTILS_T_H_
