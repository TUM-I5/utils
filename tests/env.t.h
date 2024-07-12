// SPDX-FileCopyrightText: 2014-2024 Technical University of Munich
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef UTILS_TESTS_ENV_T_H_
#define UTILS_TESTS_ENV_T_H_

#include <cstdlib>
#include <cstring>

#include "utils/env.h"

using namespace utils;

class TestEnv : public CxxTest::TestSuite {
public:
  void testGet() {
    TS_ASSERT_EQUALS(setenv("UTILS_INT", "42", 1), 0);
    TS_ASSERT_EQUALS(Env::get<int>("UTILS_INT", 0), 42);
    TS_ASSERT_EQUALS(Env::get("UTILS_INT", "0"), std::string("42"));
    TS_ASSERT_EQUALS(Env::get<int>("UTILS_INT2", 3), 3);

    TS_ASSERT_EQUALS(setenv("UTILS_BOOL", "1", 1), 0);
    TS_ASSERT_EQUALS(Env::get<bool>("UTILS_BOOL", false), true);
    TS_ASSERT_EQUALS(setenv("UTILS_BOOL2", "0", 1), 0);
    TS_ASSERT_EQUALS(Env::get<bool>("UTILS_BOOL2", false), false);
  }
};
#endif // UTILS_TESTS_ENV_T_H_
