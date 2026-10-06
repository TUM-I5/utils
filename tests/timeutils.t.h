// SPDX-FileCopyrightText: 2014-2024 Technical University of Munich
//
// SPDX-License-Identifier: BSD-3-Clause
//
// SPDX-FileContributor: Sebastian Rettenberger

#ifndef UTILS_TESTS_TIMEUTILS_T_H_
#define UTILS_TESTS_TIMEUTILS_T_H_

#include "utils/timeutils.h"

#include <ctime>
#include <string>

using namespace utils;

class TestTimeUtils : public CxxTest::TestSuite {
  // 2001-09-09T01:46:40Z; the year is the same in all time zones
  static constexpr time_t Time = 1000000000;

  public:
  void testTimeAsString() { TS_ASSERT_EQUALS(TimeUtils::timeAsString("%Y", Time), "2001"); }

  void testTimeAsStringLong() {
    std::string format;
    std::string expected;
    for (int i = 0; i < 100; ++i) {
      format += "%Y-";
      expected += "2001-";
    }
    TS_ASSERT_EQUALS(TimeUtils::timeAsString(format, Time), expected);
  }

  void testTimeAsStringEmpty() {
    // an empty result needs to terminate as well
    TS_ASSERT_EQUALS(TimeUtils::timeAsString("", Time), "");
  }
};
#endif // UTILS_TESTS_TIMEUTILS_T_H_
