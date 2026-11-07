// SPDX-FileCopyrightText: 2014-2026 Technical University of Munich
//
// SPDX-License-Identifier: BSD-3-Clause
//
// SPDX-FileContributor: Sebastian Rettenberger
// SPDX-FileContributor: David Schneller

#ifndef UTILS_TESTS_LOGGER_T_H_
#define UTILS_TESTS_LOGGER_T_H_

#include <csignal>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#if defined(__unix__) || defined(__APPLE__)
#include <sys/wait.h>
#include <unistd.h>
#define UTILS_TESTS_HAVE_FORK 1
#endif

#include "utils/logger.h"

using namespace utils;

namespace logger_test {

struct Captured {
  std::string out;
  std::string err;
};

// Runs function with std::cout and std::cerr redirected, and returns what it wrote to them.
template <typename F>
auto capture(F&& function) -> Captured {
  std::stringstream out;
  std::stringstream err;
  auto* const previousOut = std::cout.rdbuf(out.rdbuf());
  auto* const previousErr = std::cerr.rdbuf(err.rdbuf());
  std::forward<F>(function)();
  std::cout.rdbuf(previousOut);
  std::cerr.rdbuf(previousErr);
  return {out.str(), err.str()};
}

// The time stamp a line starts with, e.g. "2026-11-07 11:25:00.123".
inline auto timeStamp() -> const std::regex& {
  static const std::regex Pattern(
      "^[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}\\.[0-9]{3}");
  return Pattern;
}

// The text with the time stamp at its start removed; empty if there is none.
inline auto withoutTime(const std::string& text) -> std::string {
  std::smatch match;
  if (!std::regex_search(text, match, timeStamp())) {
    return "";
  }
  return text.substr(match.length(0));
}

#ifdef UTILS_TESTS_HAVE_FORK
struct Child {
  int status{0};
  std::string err;
};

// Runs function in a child process and returns how the child ended and what it wrote to stderr.
template <typename F>
auto runInChild(F&& function) -> Child {
  int pipeEnds[2];
  if (pipe(pipeEnds) != 0) {
    return {-1, ""};
  }
  std::cout.flush();
  std::cerr.flush();
  const pid_t pid = fork();
  if (pid == 0) {
    close(pipeEnds[0]);
    dup2(pipeEnds[1], STDERR_FILENO);
    close(pipeEnds[1]);
    std::forward<F>(function)();
    _exit(0);
  }
  close(pipeEnds[1]);
  std::string err;
  char buffer[256];
  ssize_t count = 0;
  while ((count = read(pipeEnds[0], buffer, sizeof(buffer))) > 0) {
    err.append(buffer, static_cast<std::size_t>(count));
  }
  close(pipeEnds[0]);
  int status = 0;
  waitpid(pid, &status, 0);
  return {status, err};
}
#endif

// Defined in logger_level.cpp, which is compiled with LOG_LEVEL=1.
void logAtLevelOne();

struct Streamable {
  int value;
};

inline auto operator<<(std::ostream& stream, const Streamable& streamable) -> std::ostream& {
  return stream << "Streamable(" << streamable.value << ")";
}

} // namespace logger_test

class TestLogger : public CxxTest::TestSuite {
  public:
  void setUp() override {
    Logger::setRank(0);
    Logger::setDisplayRank(0);
    Logger::setLogAll(false);
  }

  void tearDown() override { setUp(); }

  static void testTimeStamp() {
    const auto captured = logger_test::capture([] { logInfo() << "text"; });
    TS_ASSERT(std::regex_search(captured.out, logger_test::timeStamp()));
  }

  static void testInfoGoesToStdout() {
    const auto captured =
        logger_test::capture([] { logInfo() << "The mesh has" << 1234 << "cells"; });
    TS_ASSERT_EQUALS(logger_test::withoutTime(captured.out),
                     std::string(" info 0 : The mesh has 1234 cells \n"));
    TS_ASSERT_EQUALS(captured.err, std::string(""));
  }

  static void testWarningGoesToStderr() {
    const auto captured = logger_test::capture([] { logWarning() << "careful"; });
    TS_ASSERT_EQUALS(captured.out, std::string(""));
    TS_ASSERT_EQUALS(logger_test::withoutTime(captured.err), std::string(" warn 0 : careful \n"));
  }

  static void testDebugGoesToStdout() {
#if LOG_LEVEL >= 3
    const auto captured = logger_test::capture([] { logDebug() << "details"; });
    TS_ASSERT_EQUALS(logger_test::withoutTime(captured.out), std::string(" debug 0 : details \n"));
#endif
  }

  static void testSpacing() {
    const auto captured =
        logger_test::capture([] { logInfo() << "a" << 1 << 'b' << 2.5 << -3 << 7U; });
    TS_ASSERT_EQUALS(logger_test::withoutTime(captured.out),
                     std::string(" info 0 : a 1 b 2.5 -3 7 \n"));
  }

  static void testStringsAreQuoted() {
    const auto captured = logger_test::capture([] {
      const std::string dynamic = "text";
      logInfo() << dynamic << "literal" << dynamic.c_str();
    });
    TS_ASSERT_EQUALS(logger_test::withoutTime(captured.out),
                     std::string(" info 0 : \"text\" literal text \n"));
  }

  static void testNospaceAndSpace() {
    const auto captured = logger_test::capture(
        [] { logInfo() << utils::nospace << "a" << "b" << utils::space << "c" << "d"; });
    TS_ASSERT_EQUALS(logger_test::withoutTime(captured.out), std::string(" info 0 : ab c d \n"));
  }

  static void testIterables() {
    const auto captured = logger_test::capture([] {
      logInfo() << "values" << std::vector<int>{1, 2, 3} << "empty" << std::vector<int>{};
    });
    TS_ASSERT_EQUALS(logger_test::withoutTime(captured.out),
                     std::string(" info 0 : values [1, 2, 3] empty [] \n"));
  }

  static void testNestedIterables() {
    const auto captured = logger_test::capture(
        [] { logInfo() << std::vector<std::vector<int>>{{1}, {2, 3}} << "end"; });
    TS_ASSERT_EQUALS(logger_test::withoutTime(captured.out),
                     std::string(" info 0 : [[1] ,  [2, 3] ]  end \n"));
  }

  static void testTuples() {
    const auto captured = logger_test::capture(
        [] { logInfo() << std::make_pair(1, std::string("x")) << std::make_tuple(2.5, 'c', 3); });
    TS_ASSERT_EQUALS(logger_test::withoutTime(captured.out),
                     std::string(" info 0 : {1, \"x\"} {2.5, c, 3} \n"));
  }

  static void testStreamableTypes() {
    const auto captured =
        logger_test::capture([] { logInfo() << logger_test::Streamable{5} << true << false; });
    TS_ASSERT_EQUALS(logger_test::withoutTime(captured.out),
                     std::string(" info 0 : Streamable(5) 1 0 \n"));
  }

  static void testStreamManipulators() {
    const auto endline = logger_test::capture([] { logInfo() << "a" << std::endl << "b"; });
    TS_ASSERT_EQUALS(logger_test::withoutTime(endline.out), std::string(" info 0 : a \nb \n"));

    const auto precision =
        logger_test::capture([] { logInfo() << std::setprecision(3) << 1.23456; });
    TS_ASSERT_EQUALS(logger_test::withoutTime(precision.out), std::string(" info 0 :  1.23 \n"));
  }

  static void testOnlyTheDisplayRankPrints() {
    Logger::setRank(1);
    const auto hidden = logger_test::capture([] {
      logInfo() << "hidden";
      logWarning() << "hidden";
    });
    TS_ASSERT_EQUALS(hidden.out, std::string(""));
    TS_ASSERT_EQUALS(hidden.err, std::string(""));

    const auto broadcast = logger_test::capture([] {
      logInfo(true) << "shown";
      logWarning(true) << "shown";
    });
    TS_ASSERT_EQUALS(logger_test::withoutTime(broadcast.out), std::string(" info 1 : shown \n"));
    TS_ASSERT_EQUALS(logger_test::withoutTime(broadcast.err), std::string(" warn 1 : shown \n"));

    Logger::setDisplayRank(1);
    const auto display = logger_test::capture([] { logInfo() << "shown"; });
    TS_ASSERT_EQUALS(logger_test::withoutTime(display.out), std::string(" info 1 : shown \n"));
  }

  static void testLogAll() {
    Logger::setRank(3);
    Logger::setLogAll(true);
    const auto captured = logger_test::capture([] { logInfo() << "shown"; });
    TS_ASSERT_EQUALS(logger_test::withoutTime(captured.out), std::string(" info 3 : shown \n"));
  }

  static void testUnknownRankPrints() {
    Logger::setRank(-1);
    Logger::setDisplayRank(0);
    const auto captured = logger_test::capture([] { logInfo() << "early"; });
    TS_ASSERT_EQUALS(logger_test::withoutTime(captured.out), std::string(" info - : early \n"));
  }

  static void testLevelsPerTranslationUnit() {
    // both translation units use logDebug() and logInfo(), with different levels
    const auto here = logger_test::capture([] {
      logDebug() << "debug here";
      logInfo() << "info here";
    });
#if LOG_LEVEL >= 3
    TS_ASSERT_EQUALS(logger_test::withoutTime(here.out).rfind(" debug 0 : debug here \n", 0), 0U);
#endif
    TS_ASSERT_DIFFERS(here.out.find(" info 0 : info here \n"), std::string::npos);

    const auto there = logger_test::capture([] { logger_test::logAtLevelOne(); });
    TS_ASSERT_EQUALS(there.out, std::string(""));
    TS_ASSERT_EQUALS(logger_test::withoutTime(there.err),
                     std::string(" warn 0 : warning at level 1 \n"));
  }

  static void testErrorEndsTheProcess() {
#ifdef UTILS_TESTS_HAVE_FORK
    const auto child = logger_test::runInChild([] { logError() << "the error" << 42; });
    TS_ASSERT(WIFSIGNALED(child.status));
    TS_ASSERT_EQUALS(logger_test::withoutTime(child.err).rfind(" error 0 : the error 42 \n", 0),
                     0U);
#endif
  }
};

#endif // UTILS_TESTS_LOGGER_T_H_
