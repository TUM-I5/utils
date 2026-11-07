// SPDX-FileCopyrightText: 2014-2026 Technical University of Munich
//
// SPDX-License-Identifier: BSD-3-Clause
//
// SPDX-FileContributor: Sebastian Rettenberger
// SPDX-FileContributor: David Schneller

#ifndef UTILS_TESTS_LOGGER_T_H_
#define UTILS_TESTS_LOGGER_T_H_

#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
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

// Defined in logger_noreturn.cpp, which is compiled with -Werror=return-type.
auto positiveOrFatal(int value) -> int;

template <typename T>
auto holds(const LogValue& value, const T& expected) -> bool {
  const auto* held = std::get_if<T>(&value.value);
  return held != nullptr && *held == expected;
}

// The record of the one message that function makes, with the console off.
template <typename F>
auto recordOf(F&& function) -> LogRecord;

// Keeps what a callback gets, for as long as it exists.
class Collector {
  public:
  explicit Collector(LogLevel minimum = LogLevel::Debug)
      : id(Logger::addCallback([this](const LogRecord& record) { records.push_back(record); },
                               minimum)) {}
  ~Collector() { Logger::removeCallback(id); }
  Collector(const Collector&) = delete;
  Collector(Collector&&) = delete;
  auto operator=(const Collector&) -> Collector& = delete;
  auto operator=(Collector&&) -> Collector& = delete;

  std::vector<LogRecord> records;

  private:
  int id;
};

template <typename F>
auto recordOf(F&& function) -> LogRecord {
  Logger::enableConsole(false);
  const Collector collector;
  std::forward<F>(function)();
  Logger::enableConsole(true);
  return collector.records.size() == 1 ? collector.records.front() : LogRecord{};
}

// The lines of a file.
inline auto readLines(const std::string& path) -> std::vector<std::string> {
  std::ifstream file(path);
  std::vector<std::string> lines;
  std::string line;
  while (std::getline(file, line)) {
    lines.push_back(line);
  }
  return lines;
}

// A file for a test: removed if it is there already, and again at the end of the test.
class TestFile {
  public:
  explicit TestFile(const std::string& name) : path("logger_test_" + name + ".jsonl") { remove(); }
  ~TestFile() { remove(); }
  TestFile(const TestFile&) = delete;
  TestFile(TestFile&&) = delete;
  auto operator=(const TestFile&) -> TestFile& = delete;
  auto operator=(TestFile&&) -> TestFile& = delete;

  [[nodiscard]] auto name() const -> const std::string& { return path; }

  private:
  void remove() const { static_cast<void>(std::remove(path.c_str())); }

  std::string path;
};

inline auto contains(const std::string& text, const std::string& part) -> bool {
  return text.find(part) != std::string::npos;
}

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
    Logger::setLevel(LogLevel::Debug);
    Logger::setAbortHandler(nullptr);
    Logger::setTrap(Logger::Trap::Never);
    Logger::setBacktraceDepth(50);
    Logger::enableConsole(true);
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

  static void testRuntimeLevel() {
    Logger::setLevel(LogLevel::Warning);
    const auto captured = logger_test::capture([] {
      logDebug() << "hidden";
      logInfo() << "hidden";
      logWarning() << "shown";
    });
    TS_ASSERT_EQUALS(captured.out, std::string(""));
    TS_ASSERT_EQUALS(logger_test::withoutTime(captured.err), std::string(" warn 0 : shown \n"));
  }

  static void testCallbackGetsRecords() {
    logger_test::Collector collector;
    const auto captured = logger_test::capture([] {
      logInfo() << "a" << 1;
      logWarning(true) << "w";
    });
    TS_ASSERT_EQUALS(collector.records.size(), 2U);
    if (collector.records.size() == 2) {
      TS_ASSERT_EQUALS(collector.records[0].level, LogLevel::Info);
      TS_ASSERT_EQUALS(collector.records[0].text, std::string("a 1 "));
      TS_ASSERT_EQUALS(collector.records[0].rank, 0);
      TS_ASSERT(!collector.records[0].broadcast);
      TS_ASSERT_EQUALS(collector.records[1].level, LogLevel::Warning);
      TS_ASSERT(collector.records[1].broadcast);
    }
    // the console shows the messages as well
    TS_ASSERT_EQUALS(logger_test::withoutTime(captured.out), std::string(" info 0 : a 1 \n"));
  }

  static void testCallbackMinimumLevel() {
    const logger_test::Collector collector(LogLevel::Warning);
    const auto captured = logger_test::capture([] {
      logInfo() << "info";
      logWarning() << "warning";
    });
    TS_ASSERT_EQUALS(collector.records.size(), 1U);
    TS_ASSERT_EQUALS(collector.records.at(0).level, LogLevel::Warning);
  }

  static void testCallbackSeesMessagesOfAllRanks() {
    Logger::setRank(1);
    const logger_test::Collector collector;
    const auto captured = logger_test::capture([] { logInfo() << "not on this console"; });
    TS_ASSERT_EQUALS(captured.out, std::string(""));
    TS_ASSERT_EQUALS(collector.records.size(), 1U);
    TS_ASSERT_EQUALS(collector.records.at(0).rank, 1);
  }

  static void testRemoveCallback() {
    static int calls = 0;
    const int id = Logger::addCallback([](const LogRecord& /*record*/) { ++calls; });
    const auto first = logger_test::capture([] { logInfo() << "counted"; });
    Logger::removeCallback(id);
    const auto second = logger_test::capture([] { logInfo() << "not counted"; });
    TS_ASSERT_EQUALS(calls, 1);
    // removing it again does nothing
    Logger::removeCallback(id);
  }

  static void testConsoleOff() {
    Logger::enableConsole(false);
    const logger_test::Collector collector;
    const auto captured = logger_test::capture([] {
      logInfo() << "info";
      logWarning() << "warning";
    });
    TS_ASSERT_EQUALS(captured.out, std::string(""));
    TS_ASSERT_EQUALS(captured.err, std::string(""));
    TS_ASSERT_EQUALS(collector.records.size(), 2U);
  }

  static void testThrowingCallback() {
    const int id =
        Logger::addCallback([](const LogRecord& /*record*/) { throw std::runtime_error("no"); });
    const logger_test::Collector collector;
    const auto captured = logger_test::capture([] { logInfo() << "goes on"; });
    Logger::removeCallback(id);
    TS_ASSERT_EQUALS(collector.records.size(), 1U);
  }

  static void testCallbackThatLogs() {
    const int id = Logger::addCallback([](const LogRecord& record) {
      if (record.text == "outer ") {
        logInfo() << "from the callback";
      }
    });
    const logger_test::Collector collector;
    const auto captured = logger_test::capture([] { logInfo() << "outer"; });
    Logger::removeCallback(id);
    TS_ASSERT_DIFFERS(captured.out.find(" info 0 : outer \n"), std::string::npos);
    TS_ASSERT_DIFFERS(captured.out.find(" info 0 : from the callback \n"), std::string::npos);
    // the message of the callback goes to the console only
    TS_ASSERT_EQUALS(collector.records.size(), 1U);
  }

  static void testCallbackThatRemovesItself() {
    static int id = 0;
    static int calls = 0;
    id = Logger::addCallback([](const LogRecord& /*record*/) {
      ++calls;
      Logger::removeCallback(id);
    });
    const auto captured = logger_test::capture([] {
      logInfo() << "first";
      logInfo() << "second";
    });
    TS_ASSERT_EQUALS(calls, 1);
  }

  static void testFatalErrorGoesToCallbacksFirst() {
#ifdef UTILS_TESTS_HAVE_FORK
    const auto child = logger_test::runInChild([] {
      Logger::addCallback([](const LogRecord& record) {
        std::fprintf(stderr, "callback: %s\n", record.text.c_str());
      });
      Logger::setAbortHandler([](const LogRecord& /*record*/) {
        std::fprintf(stderr, "handler\n");
        static_cast<void>(std::fflush(stderr));
        std::_Exit(5);
      });
      logError() << "the error";
    });
    TS_ASSERT(WIFEXITED(child.status));
    TS_ASSERT_EQUALS(WEXITSTATUS(child.status), 5);
    const auto callback = child.err.find("callback: the error \n");
    const auto handler = child.err.find("handler\n");
    TS_ASSERT_DIFFERS(callback, std::string::npos);
    TS_ASSERT_DIFFERS(handler, std::string::npos);
    TS_ASSERT_LESS_THAN(callback, handler);
#endif
  }

  static void testTemplateFromLiterals() {
    const auto record = logger_test::recordOf(
        [] { logInfo() << "The mesh has" << 1234 << "cells and" << 567 << "vertices."; });
    TS_ASSERT_EQUALS(record.messageTemplate, std::string("The mesh has {} cells and {} vertices."));
    TS_ASSERT_EQUALS(record.arguments.size(), 2U);
    TS_ASSERT(logger_test::holds(record.arguments.at(0), std::int64_t{1234}));
    TS_ASSERT(logger_test::holds(record.arguments.at(1), std::int64_t{567}));
    TS_ASSERT_EQUALS(record.message(),
                     std::string_view("The mesh has 1234 cells and 567 vertices."));
  }

  static void testTemplateValueTypes() {
    const auto record = logger_test::recordOf([] {
      logInfo() << "x" << std::string("s") << 2.5 << true << 7U << -3 << ':'
                << std::vector<int>{1, 2} << std::make_pair(1, "a");
    });
    TS_ASSERT_EQUALS(record.messageTemplate, std::string("x {} {} {} {} {} : {} {}"));
    TS_ASSERT_EQUALS(record.arguments.size(), 7U);
    TS_ASSERT(logger_test::holds(record.arguments.at(0), std::string("s")));
    TS_ASSERT(logger_test::holds(record.arguments.at(1), 2.5));
    TS_ASSERT(logger_test::holds(record.arguments.at(2), true));
    TS_ASSERT(logger_test::holds(record.arguments.at(3), std::uint64_t{7}));
    TS_ASSERT(logger_test::holds(record.arguments.at(4), std::int64_t{-3}));
    const auto* list = std::get_if<LogValue::List>(&record.arguments.at(5).value);
    TS_ASSERT(list != nullptr && list->size() == 2 &&
              logger_test::holds(list->at(1), std::int64_t{2}));
    const auto* pair = std::get_if<LogValue::List>(&record.arguments.at(6).value);
    TS_ASSERT(pair != nullptr && pair->size() == 2 &&
              logger_test::holds(pair->at(1), std::string("a")));
  }

  static void testDynamicTextIsAValue() {
    const auto record = logger_test::recordOf([] {
      const std::string dynamic = "dyn";
      char changing[] = "buf";
      logInfo() << "a" << dynamic.c_str() << changing;
    });
    TS_ASSERT_EQUALS(record.messageTemplate, std::string("a {} {}"));
    TS_ASSERT_EQUALS(record.arguments.size(), 2U);
    TS_ASSERT(logger_test::holds(record.arguments.at(0), std::string("dyn")));
    TS_ASSERT(logger_test::holds(record.arguments.at(1), std::string("buf")));
  }

  static void testBracesOfTheTextAreDoubled() {
    const auto record = logger_test::recordOf([] { logInfo() << "set {a}" << 1 << '}'; });
    TS_ASSERT_EQUALS(record.messageTemplate, std::string("set {{a}} {} }}"));
  }

  static void testManipulatorsAreNoValues() {
    const auto record =
        logger_test::recordOf([] { logInfo() << std::setprecision(3) << 1.23456 << std::endl; });
    TS_ASSERT_EQUALS(record.text, std::string(" 1.23 \n"));
    TS_ASSERT_EQUALS(record.messageTemplate, std::string(" {} \n"));
    TS_ASSERT_EQUALS(record.arguments.size(), 1U);
    // the value keeps its precision
    TS_ASSERT(logger_test::holds(record.arguments.at(0), 1.23456));
  }

  static void testNestedValuesAreOneValue() {
    const auto record = logger_test::recordOf(
        [] { logInfo() << std::vector<std::vector<int>>{{1}, {2, 3}} << "end"; });
    TS_ASSERT_EQUALS(record.messageTemplate, std::string("{} end"));
    TS_ASSERT_EQUALS(record.arguments.size(), 1U);
  }

  static void testStreamableValueAsText() {
    const auto record = logger_test::recordOf([] { logInfo() << logger_test::Streamable{5}; });
    TS_ASSERT_EQUALS(record.messageTemplate, std::string("{}"));
    TS_ASSERT(logger_test::holds(record.arguments.at(0), std::string("Streamable(5)")));
  }

  static void testFields() {
    const auto record = logger_test::recordOf([] {
      logInfo() << "The mesh has" << utils::field("cells", 1234) << "cells in"
                << utils::field("file", "mesh.h5");
    });
    TS_ASSERT_EQUALS(record.text, std::string("The mesh has 1234 cells in mesh.h5 "));
    TS_ASSERT_EQUALS(record.messageTemplate, std::string("The mesh has {} cells in {}"));
    TS_ASSERT_EQUALS(record.arguments.size(), 2U);
    TS_ASSERT_EQUALS(record.fields.size(), 2U);
    TS_ASSERT_EQUALS(record.fields.at(0).first, std::string("cells"));
    TS_ASSERT(logger_test::holds(record.fields.at(0).second, std::int64_t{1234}));
    TS_ASSERT_EQUALS(record.fields.at(1).first, std::string("file"));
    TS_ASSERT(logger_test::holds(record.fields.at(1).second, std::string("mesh.h5")));
  }

  static void testContextFields() {
    Logger::setField("run", std::string("abc"));
    Logger::setField("attempt", 1);
    Logger::setField("run", std::string("def"));
    const auto record = logger_test::recordOf([] { logInfo() << "x"; });
    TS_ASSERT_EQUALS(record.context.size(), 2U);
    TS_ASSERT_EQUALS(record.context.at(0).first, std::string("run"));
    TS_ASSERT(logger_test::holds(record.context.at(0).second, std::string("def")));
    Logger::removeField("run");
    Logger::removeField("attempt");
    const auto after = logger_test::recordOf([] { logInfo() << "x"; });
    TS_ASSERT(after.context.empty());
  }

  static void testLocation() {
    Logger::enableConsole(false);
    const logger_test::Collector collector;
    const std::uint32_t line = __LINE__ + 1;
    logInfo() << "here";
    Logger::enableConsole(true);
    TS_ASSERT_EQUALS(collector.records.size(), 1U);
    const auto& location = collector.records.at(0).location;
    TS_ASSERT_EQUALS(location.line, line);
    TS_ASSERT(std::string(location.file).find("logger.t.h") != std::string::npos);
    TS_ASSERT(std::string(location.function).find("testLocation") != std::string::npos);
  }

  static void testJsonLines() {
    const logger_test::TestFile file("lines");
    const auto& path = file.name();
    Logger::keepEarlyRecords(0);
    TS_ASSERT(Logger::enableJsonLines(path));
    Logger::setField("run", std::string("abc"));
    const auto captured = logger_test::capture([] {
      logInfo() << "The mesh has" << utils::field("cells", 1234) << "cells.";
      logWarning() << "quote \" backslash \\ newline\n tab\t";
      logInfo() << 2.5 << 0.1 << 1e300 << std::nan("") << -1.0 / 0.0 << std::vector<int>{1, 2};
    });
    Logger::removeField("run");
    Logger::disableJsonLines();

    const auto lines = logger_test::readLines(path);
    TS_ASSERT_EQUALS(lines.size(), 3U);
    if (lines.size() != 3) {
      return;
    }
    const std::regex time(
        "^\\{\"time\":\"[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}\\.[0-9]{6}Z\",");
    TS_ASSERT(std::regex_search(lines[0], time));
    TS_ASSERT(logger_test::contains(
        lines[0],
        ",\"level\":\"info\",\"rank\":0,\"run\":\"abc\",\"message\":\"The mesh has 1234 "
        "cells.\",\"template\":\"The mesh has {} cells.\",\"arguments\":[1234],\"fields\":{"
        "\"cells\":1234},\"source\":{\"file\":\""));
    TS_ASSERT(logger_test::contains(lines[0], "logger.t.h\",\"line\":"));
    TS_ASSERT(logger_test::contains(lines[0], ",\"function\":\"operator()\"}}"));
    TS_ASSERT(logger_test::contains(lines[1], "\"level\":\"warning\""));
    TS_ASSERT(logger_test::contains(lines[1],
                                    "\"message\":\"quote \\\" backslash \\\\ newline\\n tab\\t\""));
    TS_ASSERT(logger_test::contains(lines[2],
                                    "\"arguments\":[2.5,0.1,1e+300,\"NaN\",\"-Infinity\",[1,2]]"));
  }

  static void testJsonLinesOnlyOnTheDisplayRank() {
    const logger_test::TestFile file("rank");
    const auto& path = file.name();
    Logger::setRank(1);
    TS_ASSERT(!Logger::enableJsonLines(path));
    const std::ifstream opened(path);
    TS_ASSERT(!opened.good());
  }

  static void testJsonLinesAppend() {
    const logger_test::TestFile file("append");
    const auto& path = file.name();
    const auto captured = logger_test::capture([&path] {
      Logger::enableJsonLines(path);
      logInfo() << "first";
      Logger::disableJsonLines();
      Logger::enableJsonLines(path);
      logInfo() << "second";
      Logger::disableJsonLines();
    });
    const auto lines = logger_test::readLines(path);
    TS_ASSERT_EQUALS(lines.size(), 2U);
  }

  static void testJsonLinesWithoutConsole() {
    const logger_test::TestFile file("noconsole");
    const auto& path = file.name();
    Logger::enableConsole(false);
    Logger::enableJsonLines(path);
    const auto captured = logger_test::capture([] { logInfo() << "only in the file"; });
    Logger::disableJsonLines();
    TS_ASSERT_EQUALS(captured.out, std::string(""));
    const auto lines = logger_test::readLines(path);
    TS_ASSERT_EQUALS(lines.size(), 1U);
    TS_ASSERT(logger_test::contains(lines.at(0), "\"message\":\"only in the file\""));
  }

  static void testEarlyRecords() {
#ifdef UTILS_TESTS_HAVE_FORK
    const logger_test::TestFile file("early");
    const auto& path = file.name();
    const auto child = logger_test::runInChild([&path] {
      Logger::disableJsonLines();
      Logger::keepEarlyRecords(0);
      Logger::keepEarlyRecords(10);
      Logger::setRank(-1);
      logInfo() << "early";
      Logger::setRank(0);
      Logger::enableJsonLines(path);
      logInfo() << "late";
      Logger::disableJsonLines();
    });
    TS_ASSERT(WIFEXITED(child.status));
    const auto lines = logger_test::readLines(path);
    TS_ASSERT_EQUALS(lines.size(), 2U);
    if (lines.size() == 2) {
      TS_ASSERT(logger_test::contains(lines[0], "\"rank\":0,\"message\":\"early\""));
      TS_ASSERT(logger_test::contains(lines[1], "\"message\":\"late\""));
    }
#endif
  }

  static void testFatalErrorInJsonLines() {
#ifdef UTILS_TESTS_HAVE_FORK
    const logger_test::TestFile file("fatal");
    const auto& path = file.name();
    const auto child = logger_test::runInChild([&path] {
      Logger::enableJsonLines(path);
      Logger::setAbortHandler([](const LogRecord& /*record*/) { std::_Exit(0); });
      logError() << "fatal end";
    });
    TS_ASSERT(WIFEXITED(child.status));
    const auto lines = logger_test::readLines(path);
    TS_ASSERT_EQUALS(lines.size(), 1U);
    if (lines.size() == 1) {
      TS_ASSERT(logger_test::contains(lines[0], "\"level\":\"error\""));
      TS_ASSERT(logger_test::contains(lines[0], "\"message\":\"fatal end\""));
    }
#endif
  }

  static void testReservedFieldNames() {
    const auto captured = logger_test::capture([] { Logger::setField("message", 1); });
    TS_ASSERT_EQUALS(
        logger_test::withoutTime(captured.err),
        std::string(" warn 0 : The field \"message\" is not set: the JSON lines have a key of that "
                    "name. \n"));
    const auto record = logger_test::recordOf([] { logInfo() << "x"; });
    TS_ASSERT(record.context.empty());
  }

  static void testFatalErrorAborts() {
#ifdef UTILS_TESTS_HAVE_FORK
    const auto child = logger_test::runInChild([] { logFatal() << "fatal"; });
    TS_ASSERT(WIFSIGNALED(child.status));
    TS_ASSERT_EQUALS(WTERMSIG(child.status), SIGABRT);
    TS_ASSERT_EQUALS(logger_test::withoutTime(child.err).rfind(" error 0 : fatal \n", 0), 0U);
#endif
  }

  static void testAbortHandlerGetsTheRecord() {
#ifdef UTILS_TESTS_HAVE_FORK
    const auto child = logger_test::runInChild([] {
      Logger::setAbortHandler([](const LogRecord& record) {
        std::fprintf(stderr,
                     "handler: [%s] %d\n",
                     record.text.c_str(),
                     static_cast<int>(record.level == LogLevel::Error));
        static_cast<void>(std::fflush(stderr));
        std::_Exit(42);
      });
      logError() << "the error" << 7;
    });
    TS_ASSERT(WIFEXITED(child.status));
    TS_ASSERT_EQUALS(WEXITSTATUS(child.status), 42);
    TS_ASSERT_DIFFERS(child.err.find("handler: [the error 7 ] 1\n"), std::string::npos);
#endif
  }

  static void testAbortHandlerThatReturns() {
#ifdef UTILS_TESTS_HAVE_FORK
    const auto child = logger_test::runInChild([] {
      Logger::setAbortHandler([](const LogRecord& /*record*/) {});
      logError() << "the error";
    });
    TS_ASSERT(WIFSIGNALED(child.status));
    TS_ASSERT_EQUALS(WTERMSIG(child.status), SIGABRT);
#endif
  }

  static void testFatalErrorInTheAbortHandler() {
#ifdef UTILS_TESTS_HAVE_FORK
    const auto child = logger_test::runInChild([] {
      Logger::setAbortHandler([](const LogRecord& /*record*/) { logError() << "in the handler"; });
      logError() << "the error";
    });
    TS_ASSERT(WIFSIGNALED(child.status));
    TS_ASSERT_EQUALS(WTERMSIG(child.status), SIGABRT);
    TS_ASSERT_DIFFERS(child.err.find(" error 0 : in the handler \n"), std::string::npos);
#endif
  }

  static void testTrap() {
#if defined(UTILS_TESTS_HAVE_FORK) && defined(SIGTRAP)
    const auto always = logger_test::runInChild([] {
      Logger::setTrap(Logger::Trap::Always);
      logError() << "the error";
    });
    TS_ASSERT(WIFSIGNALED(always.status));
    TS_ASSERT_EQUALS(WTERMSIG(always.status), SIGTRAP);

    const auto never = logger_test::runInChild([] {
      Logger::setTrap(Logger::Trap::Never);
      logError() << "the error";
    });
    TS_ASSERT(WIFSIGNALED(never.status));
    TS_ASSERT_EQUALS(WTERMSIG(never.status), SIGABRT);
#endif
  }

  static void testBacktraceDepth() {
#ifdef UTILS_TESTS_HAVE_FORK
#ifdef UTILS_LOGGER_HAS_BACKTRACE
    const auto with = logger_test::runInChild([] { logError() << "the error"; });
    TS_ASSERT_DIFFERS(with.err.find("Backtrace:\n"), std::string::npos);
#endif
    const auto without = logger_test::runInChild([] {
      Logger::setBacktraceDepth(0);
      logError() << "the error";
    });
    TS_ASSERT_EQUALS(without.err.find("Backtrace:"), std::string::npos);
#endif
  }

  static void testSetAbortHandlerReturnsThePrevious() {
    static int calls = 0;
    Logger::setAbortHandler([](const LogRecord& /*record*/) { ++calls; });
    const auto previous = Logger::setAbortHandler(nullptr);
    TS_ASSERT(static_cast<bool>(previous));
    previous(LogRecord{});
    TS_ASSERT_EQUALS(calls, 1);
    TS_ASSERT(!static_cast<bool>(Logger::setAbortHandler(nullptr)));
  }

  static void testNoReturnAfterAFatalError() {
    TS_ASSERT_EQUALS(logger_test::positiveOrFatal(3), 3);
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
