// SPDX-FileCopyrightText: 2014-2024 Technical University of Munich
//
// SPDX-License-Identifier: BSD-3-Clause
//
// SPDX-FileContributor: Sebastian Rettenberger

#ifndef UTILS_TESTS_ENV_T_H_
#define UTILS_TESTS_ENV_T_H_

#include <cstdlib>
#include <cstring>
#include <string>

#include "utils/env.h"

using namespace utils;

class TestEnv : public CxxTest::TestSuite {
  public:
  static void testGet() {
    Env env("UTILS_");
    TS_ASSERT_EQUALS(setenv("UTILS_INT", "42", 1), 0);
    TS_ASSERT_EQUALS(env.get<int>("INT", 0), 42);
    TS_ASSERT_EQUALS(env.get("INT", "0"), std::string("42"));
    TS_ASSERT_EQUALS(env.get<int>("INT2", 3), 3);

    TS_ASSERT_EQUALS(setenv("UTILS_BOOL", "1", 1), 0);
    TS_ASSERT_EQUALS(env.get<bool>("BOOL", false), true);
    TS_ASSERT_EQUALS(setenv("UTILS_BOOL2", "0", 1), 0);
    TS_ASSERT_EQUALS(env.get<bool>("BOOL2", false), false);
  }

  static void testPrefixChain() {
    Env env{"UTILS_NEW_", "UTILS_OLD_"};
    TS_ASSERT_EQUALS(env.prefix(), std::string("UTILS_NEW_"));

    // the canonical prefix first, then the others
    TS_ASSERT_EQUALS(setenv("UTILS_OLD_ONLY", "1", 1), 0);
    TS_ASSERT_EQUALS(setenv("UTILS_NEW_BOTH", "2", 1), 0);
    TS_ASSERT_EQUALS(setenv("UTILS_OLD_BOTH", "3", 1), 0);
    TS_ASSERT_EQUALS(env.get<int>("ONLY", 0), 1);
    TS_ASSERT_EQUALS(env.get<int>("BOTH", 0), 2);
    TS_ASSERT_EQUALS(env.get<int>("NEITHER", 4), 4);
    TS_ASSERT(!env.getOptional<int>("NEITHER").has_value());
  }

  static void testLookupOnce() {
    Env env("UTILS_ONCE_");
    TS_ASSERT_EQUALS(setenv("UTILS_ONCE_VALUE", "1", 1), 0);
    TS_ASSERT_EQUALS(env.get<int>("VALUE", 0), 1);

    TS_ASSERT_EQUALS(setenv("UTILS_ONCE_VALUE", "2", 1), 0);
    TS_ASSERT_EQUALS(env.get<int>("VALUE", 0), 1);
    TS_ASSERT_EQUALS(Env("UTILS_ONCE_").get<int>("VALUE", 0), 2);
  }
};
#endif // UTILS_TESTS_ENV_T_H_
