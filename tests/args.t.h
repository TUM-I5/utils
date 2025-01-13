// SPDX-FileCopyrightText: 2014-2024 Technical University of Munich
//
// SPDX-License-Identifier: BSD-3-Clause
//
// SPDX-FileContributor: Sebastian Rettenberger

#ifndef UTILS_TESTS_ARGS_T_H_
#define UTILS_TESTS_ARGS_T_H_

#include "utils/args.h"

using namespace utils;

class TestArgs : public CxxTest::TestSuite {
  public:
  static void testParseAdditional() {
    const char* argv[] = {"prog", "1", "2", "3"};

    Args args1("");
    args1.addAdditionalOption("test1", "", true);
    args1.addAdditionalOption("test2", "", true);
    args1.addAdditionalOption("test3", "", true);

    args1.parse(4, const_cast<char**>(argv), false);
    TS_ASSERT_EQUALS(args1.getAdditionalArgument<int>("test2"), 2);

    Args args2("");
    args2.addAdditionalOption("test1", "", true);
    args2.addAdditionalOption("test2", "", true);

    args2.parse(4, const_cast<char**>(argv), false);
    TS_ASSERT_EQUALS(args2.getAdditionalArgument<int>("test2"), 2);

    // TODO add more tests to cover all possibilities
  }
};
#endif // UTILS_TESTS_ARGS_T_H_
