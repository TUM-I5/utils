// SPDX-FileCopyrightText: 2015-2024 Technical University of Munich
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef UTILS_TESTS_ARRAYUTILS_T_H_
#define UTILS_TESTS_ARRAYUTILS_T_H_

#include <vector>

#include "utils/arrayutils.h"

using namespace utils;

class TestArgs : public CxxTest::TestSuite {
public:
  void testSize() {
    const char *a0[] = {"a", "b", "c"};
    TS_ASSERT_EQUALS(ArrayUtils::size(a0), 3);

    int a1[] = {1, 2, 3, 4, 5, 6};
    TS_ASSERT_EQUALS(ArrayUtils::size(a1), 6);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    int a2[0];
#pragma GCC diagnostic pop
    TS_ASSERT_EQUALS(ArrayUtils::size(a2), 0);

    std::vector<char> a3;
    a3.push_back('c');
    a3.push_back('d');
    TS_ASSERT_EQUALS(ArrayUtils::size(a3), 2);
  }
};
#endif // UTILS_TESTS_ARRAYUTILS_T_H_
