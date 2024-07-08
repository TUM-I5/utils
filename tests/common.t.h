// SPDX-FileCopyrightText: 2016-2024 Technical University of Munich
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef UTILS_TESTS_COMMON_T_H_
#define UTILS_TESTS_COMMON_T_H_

#include "utils/common.h"

using namespace utils;

class TestCommon : public CxxTest::TestSuite {
public:
  void testSwap() {
    int a = 1;
    int b = 3;
    utils::swap(a, b);
    TS_ASSERT_EQUALS(a, 3);
    TS_ASSERT_EQUALS(b, 1);

    int *c = &a;
    int *d = &b;
    utils::swap(c, d);
    TS_ASSERT_EQUALS(*c, 1);
    TS_ASSERT_EQUALS(*d, 3);
  }
};
#endif // UTILS_TESTS_COMMON_T_H_
