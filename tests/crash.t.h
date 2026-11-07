// SPDX-FileCopyrightText: 2026 Technical University of Munich
//
// SPDX-License-Identifier: BSD-3-Clause
//
// SPDX-FileContributor: David Schneller

#ifndef UTILS_TESTS_CRASH_T_H_
#define UTILS_TESTS_CRASH_T_H_

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <cxxtest/TestSuite.h>

#include "utils/crash.h"
#include "utils/logger.h"

#ifdef UTILS_CRASH_SIGNALS
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

using namespace utils;

#ifdef UTILS_CRASH_SIGNALS
namespace crash_test {

struct Child {
  int status{0};
  // what the child wrote to stdout and stderr
  std::string output;
};

// Runs function in a child process, without core dumps, and returns how the child ended and what
// it wrote; stdout goes to a pipe, so that it is buffered as in a batch job.
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
    const rlimit noCore{0, 0};
    setrlimit(RLIMIT_CORE, &noCore);
    close(pipeEnds[0]);
    dup2(pipeEnds[1], STDOUT_FILENO);
    dup2(pipeEnds[1], STDERR_FILENO);
    close(pipeEnds[1]);
    std::forward<F>(function)();
    std::fflush(nullptr);
    _exit(0);
  }
  close(pipeEnds[1]);
  std::string output;
  char buffer[256];
  ssize_t count = 0;
  while ((count = read(pipeEnds[0], buffer, sizeof(buffer))) > 0) {
    output.append(buffer, static_cast<std::size_t>(count));
  }
  close(pipeEnds[0]);
  int status = 0;
  waitpid(pid, &status, 0);
  return {status, output};
}

inline auto endedBy(const Child& child, int signal) -> bool {
  return WIFSIGNALED(child.status) && WTERMSIG(child.status) == signal;
}

inline auto exitedWith(const Child& child, int code) -> bool {
  return WIFEXITED(child.status) && WEXITSTATUS(child.status) == code;
}

inline auto contains(const std::string& text, const std::string& part) -> bool {
  return text.find(part) != std::string::npos;
}

inline auto readFile(const std::string& path) -> std::string {
  std::ifstream file(path);
  std::stringstream content;
  content << file.rdbuf();
  return content.str();
}

inline auto lastLine(const std::string& text) -> std::string {
  const auto end = text.find_last_not_of('\n');
  if (end == std::string::npos) {
    return {};
  }
  const auto start = text.rfind('\n', end);
  return text.substr(start == std::string::npos ? 0 : start + 1, end + 1 - (start + 1));
}

// A file for a test: removed if it is there already, and again at the end of the test.
class TestFile {
  public:
  explicit TestFile(const std::string& name) : path("crash_test_" + name + ".txt") { remove(); }
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

// Writes through a null pointer; both volatile, or the compiler leaves out the write.
inline void writeToNull() {
  volatile int* volatile pointer = nullptr;
  *pointer = 1;
}

// The handler of the tests: writes what it gets to crash_test_handler.txt, and ends with 42.
inline void writeSignal(const LogSignal& signal) {
  const int file = open("crash_test_handler.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
  for (const char* text : {signal.name, "\n", signal.message, "\n", signal.json, "\n"}) {
    std::size_t length = 0;
    while (text[length] != '\0') {
      ++length;
    }
    if (write(file, text, length) < 0) {
      _exit(41);
    }
  }
  close(file);
  _exit(42);
}

inline void throwFrom() { throw std::runtime_error("the reason"); }

inline void throwIntFrom() { throw 42; }

// Lets an exception out of a function that must not throw.
inline void escapeFrom(void (*thrower)()) noexcept { thrower(); }

} // namespace crash_test

#endif // UTILS_CRASH_SIGNALS

class TestCrash : public CxxTest::TestSuite {
  public:
  void setUp() override {
    Logger::setRank(0);
    Logger::setDisplayRank(0);
  }

  void testSegmentationFault() {
#ifdef UTILS_CRASH_SIGNALS
    const auto child = crash_test::runInChild([] {
      Logger::setRank(3);
      catchSignals();
      crash_test::writeToNull();
    });
    TS_ASSERT(crash_test::endedBy(child, SIGSEGV));
    TS_ASSERT(crash_test::contains(child.output,
                                   " error 3 : The process received the signal SIGSEGV "
                                   "(segmentation fault: address not mapped), for the address "
                                   "0x0. \n"));
    TS_ASSERT(crash_test::contains(child.output, "Backtrace:\n"));
#endif
  }

  void testStdoutIsWrittenOut() {
#ifdef UTILS_CRASH_SIGNALS
    const auto child = crash_test::runInChild([] {
      catchSignals();
      logInfo() << "Before the fault.";
      crash_test::writeToNull();
    });
    TS_ASSERT(crash_test::endedBy(child, SIGSEGV));
    TS_ASSERT(crash_test::contains(child.output, "Before the fault."));
#endif
  }

  void testHandlerGetsTheSignal() {
#ifdef UTILS_CRASH_SIGNALS
    const crash_test::TestFile file("handler");
    const auto child = crash_test::runInChild([] {
      catchSignals(&crash_test::writeSignal);
      crash_test::writeToNull();
    });
    TS_ASSERT(crash_test::exitedWith(child, 42));
    const auto written = crash_test::readFile(file.name());
    TS_ASSERT(crash_test::contains(written, "SIGSEGV\nThe process received the signal SIGSEGV"));
    TS_ASSERT(crash_test::contains(written,
                                   R"({"message":"The process received the signal SIGSEGV )"
                                   R"((segmentation fault: address not mapped), for the )"
                                   R"(address 0x0.","signal":"SIGSEGV","address":"0x0",)"
                                   R"("time":")"));
    TS_ASSERT(crash_test::contains(written, "Z\"}\n"));
#endif
  }

  void testJsonLine() {
#ifdef UTILS_CRASH_SIGNALS
    const crash_test::TestFile file("lines");
    const auto child = crash_test::runInChild([&file] {
      Logger::setField("run", "the run");
      Logger::enableJsonLines(file.name());
      catchSignals();
      // after catchSignals(), to see that the signal gets the fields as they are then
      Logger::setField("step", 7);
      crash_test::writeToNull();
    });
    TS_ASSERT(crash_test::endedBy(child, SIGSEGV));
    const auto line = crash_test::lastLine(crash_test::readFile(file.name()));
    TS_ASSERT(crash_test::contains(line, R"({"time":")"));
    TS_ASSERT(crash_test::contains(line,
                                   R"(Z","level":"error","rank":0,"run":"the run","step":7,)"
                                   R"("message":"The process received the signal SIGSEGV )"
                                   R"((segmentation fault: address not mapped), for the )"
                                   R"(address 0x0.",)"));
    TS_ASSERT(crash_test::contains(
        line,
        R"("template":"The process received the signal {} ({}), for the address {}.",)"
        R"("arguments":["SIGSEGV","segmentation fault: address not mapped","0x0"],)"
        R"("fields":{"signal":"SIGSEGV","address":"0x0"},)"
        R"("source":{"file":"","line":0,"function":""}})"));
#endif
  }

  void testAbort() {
#ifdef UTILS_CRASH_SIGNALS
    const auto child = crash_test::runInChild([] {
      catchSignals();
      std::abort();
    });
    TS_ASSERT(crash_test::endedBy(child, SIGABRT));
    TS_ASSERT(crash_test::contains(
        child.output, " error 0 : The process received the signal SIGABRT (abort). \n"));
#endif
  }

  void testFatalErrorIsReportedOnce() {
#ifdef UTILS_CRASH_SIGNALS
    const auto child = crash_test::runInChild([] {
      catchSignals();
      Logger::setAbortHandler({});
      logError() << "The fatal error.";
    });
    TS_ASSERT(crash_test::endedBy(child, SIGABRT));
    TS_ASSERT(crash_test::contains(child.output, " error 0 : The fatal error. \n"));
    TS_ASSERT(!crash_test::contains(child.output, "received the signal"));
#endif
  }

  void testSentSignal() {
#ifdef UTILS_CRASH_SIGNALS
    const auto child = crash_test::runInChild([] {
      catchSignals();
      const pid_t sender = fork();
      if (sender == 0) {
        kill(getppid(), SIGSEGV);
        _exit(0);
      }
      for (;;) {
        pause();
      }
    });
    TS_ASSERT(crash_test::endedBy(child, SIGSEGV));
    TS_ASSERT(crash_test::contains(
        child.output,
        "The process received the signal SIGSEGV (segmentation fault), sent by the process "));
#endif
  }

  void testHandlerSetBeforeIsKept() {
#ifdef UTILS_CRASH_SIGNALS
    const auto child = crash_test::runInChild([] {
      struct sigaction action{};
      action.sa_handler = [](int /*signal*/) { _exit(7); };
      sigemptyset(&action.sa_mask);
      sigaction(SIGSEGV, &action, nullptr);
      catchSignals();
      // the others are caught nonetheless
      struct sigaction abortAction{};
      sigaction(SIGABRT, nullptr, &abortAction);
      if ((abortAction.sa_flags & SA_SIGINFO) == 0) {
        _exit(8);
      }
      crash_test::writeToNull();
    });
    TS_ASSERT(crash_test::exitedWith(child, 7));
    TS_ASSERT(!crash_test::contains(child.output, "received the signal"));
#endif
  }

  // (an integer division by zero traps on x86, but not e.g. on ARM)
  void testDivisionByZero() {
#if defined(UTILS_CRASH_SIGNALS) && (defined(__x86_64__) || defined(__i386__))
    const auto child = crash_test::runInChild([] {
      catchSignals();
      // (both volatile: GCC computes 1 / x without a division)
      volatile int dividend = 7;
      volatile int zero = 0;
      volatile int quotient = dividend / zero;
      static_cast<void>(quotient);
    });
    TS_ASSERT(crash_test::endedBy(child, SIGFPE));
    TS_ASSERT(crash_test::contains(child.output,
                                   "The process received the signal SIGFPE (arithmetic error: "
                                   "integer division by zero), at the instruction 0x"));
#endif
  }

  void testUncaughtException() {
#ifdef UTILS_CRASH_SIGNALS
    const auto child = crash_test::runInChild([] {
      catchUncaughtExceptions();
      Logger::setAbortHandler([](const LogRecord& /*record*/) { _exit(43); });
      crash_test::escapeFrom(&crash_test::throwFrom);
    });
    TS_ASSERT(crash_test::exitedWith(child, 43));
    TS_ASSERT(crash_test::contains(child.output,
                                   " error 0 : An exception of type \"std::runtime_error\" was not "
                                   "caught: \"the reason\" \n"));
    TS_ASSERT(crash_test::contains(child.output, "Backtrace:\n"));
#endif
  }

  void testUncaughtExceptionOfOtherType() {
#ifdef UTILS_CRASH_SIGNALS
    const auto child = crash_test::runInChild([] {
      catchUncaughtExceptions();
      Logger::setAbortHandler([](const LogRecord& /*record*/) { _exit(43); });
      crash_test::escapeFrom(&crash_test::throwIntFrom);
    });
    TS_ASSERT(crash_test::exitedWith(child, 43));
    TS_ASSERT(crash_test::contains(child.output,
                                   " error 0 : An exception of type \"int\" was not caught. \n"));
#endif
  }

  void testTerminateWithoutException() {
#ifdef UTILS_CRASH_SIGNALS
    const auto child = crash_test::runInChild([] {
      catchUncaughtExceptions();
      Logger::setAbortHandler([](const LogRecord& /*record*/) { _exit(43); });
      std::terminate();
    });
    TS_ASSERT(crash_test::exitedWith(child, 43));
    TS_ASSERT(crash_test::contains(
        child.output, " error 0 : std::terminate() was called without an exception. \n"));
#endif
  }

  void testUncaughtExceptionEndsWithAbortWithoutHandler() {
#ifdef UTILS_CRASH_SIGNALS
    const auto child = crash_test::runInChild([] {
      catchSignals();
      catchUncaughtExceptions();
      Logger::setAbortHandler({});
      crash_test::escapeFrom(&crash_test::throwFrom);
    });
    TS_ASSERT(crash_test::endedBy(child, SIGABRT));
    TS_ASSERT(crash_test::contains(child.output, "was not caught: \"the reason\""));
    TS_ASSERT(!crash_test::contains(child.output, "received the signal"));
#endif
  }
};

#endif // UTILS_TESTS_CRASH_T_H_
