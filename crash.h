// SPDX-FileCopyrightText: 2026 Technical University of Munich
//
// SPDX-License-Identifier: BSD-3-Clause
//
// SPDX-FileContributor: David Schneller

#ifndef UTILS_CRASH_H_
#define UTILS_CRASH_H_

#include "utils/logger.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <typeinfo>

// fatal signals are caught where there is POSIX
#if defined(__unix__) || defined(__APPLE__)
#include <csignal>
#include <cstdio>
#include <ctime>
#include <signal.h>
// POSIX functions, which only the C headers declare
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#define UTILS_CRASH_SIGNALS 1
#endif

#ifdef UTILS_LOGGER_HAS_BACKTRACE
#include <execinfo.h>
#endif

#if __has_include(<cxxabi.h>)
#include <cxxabi.h>
#define UTILS_CRASH_DEMANGLE 1
#endif

namespace utils {
inline namespace logger_v3 {

/** A fatal signal, as the logger reported it */
struct LogSignal {
  /** The number of the signal, e.g. SIGSEGV */
  int number{0};
  /** Its name, e.g. "SIGSEGV" */
  const char* name{""};
  /** The message of the report, without the time stamp, the level and the rank */
  const char* message{""};
  /**
   * The signal as a JSON object: the message, the signal, the address of the fault or the process
   * that sent the signal, if there is one, and the time (ISO 8601, UTC)
   */
  const char* json{"{}"};
};

/**
 * What the signal handler of catchSignals() calls after the report of a fatal signal, e.g. to mark
 * a status file as failed, or to end the other ranks
 *
 * It runs in the signal handler, so it may only call functions that are async-signal-safe. If it
 * returns, the signal ends the program as it does by default.
 */
using LogSignalHandler = void (*)(const LogSignal&);

namespace detail {

/** The signals that catchSignals() reports */
#ifdef UTILS_CRASH_SIGNALS
constexpr std::array<int, 5> CrashSignals = {SIGSEGV, SIGBUS, SIGFPE, SIGILL, SIGABRT};
#endif

/** Text in a buffer of a fixed size, which a signal handler can put together; cut off when full */
template <std::size_t Capacity>
class FixedText {
  public:
  void clear() {
    size = 0;
    buffer[0] = '\0';
  }

  void append(std::string_view text) {
    for (const char character : text) {
      if (size + 1 < Capacity) {
        buffer[size] = character;
        ++size;
      }
    }
    buffer[size] = '\0';
  }

  /** Appends number in decimal, with at least digits digits */
  void appendNumber(std::uint64_t number, std::size_t digits = 1) {
    std::array<char, 20> reversed{};
    std::size_t count = 0;
    do {
      reversed[count] = static_cast<char>('0' + number % 10);
      ++count;
      number /= 10;
    } while (number != 0);
    while (count < digits && count < reversed.size()) {
      reversed[count] = '0';
      ++count;
    }
    while (count > 0) {
      --count;
      append(std::string_view(&reversed[count], 1));
    }
  }

  void appendSigned(std::int64_t number) {
    if (number < 0) {
      append("-");
      appendNumber(~static_cast<std::uint64_t>(number) + 1);
    } else {
      appendNumber(static_cast<std::uint64_t>(number));
    }
  }

  /** Appends number in hexadecimal, after 0x */
  void appendHex(std::uintptr_t number) {
    constexpr std::string_view Digits = "0123456789abcdef";
    std::array<char, 2 * sizeof(std::uintptr_t)> reversed{};
    std::size_t count = 0;
    do {
      reversed[count] = Digits[number % 16];
      ++count;
      number /= 16;
    } while (number != 0);
    append("0x");
    while (count > 0) {
      --count;
      append(std::string_view(&reversed[count], 1));
    }
  }

  /**
   * Appends the time of seconds since the epoch as YYYY-MM-DD, separator and hh:mm:ss, by the
   * proleptic Gregorian calendar (as std::chrono gives it, and which needs no time zone database)
   */
  void appendCalendar(std::int64_t seconds, char separator) {
    constexpr std::int64_t Day = 86400;
    std::int64_t days = seconds / Day;
    std::int64_t time = seconds % Day;
    if (time < 0) {
      time += Day;
      --days;
    }
    // from the days since the epoch to year, month and day (H. Hinnant, "chrono-Compatible
    // Low-Level Date Algorithms")
    days += 719468;
    const std::int64_t era = (days >= 0 ? days : days - 146096) / 146097;
    const auto dayOfEra = static_cast<std::uint64_t>(days - era * 146097);
    const std::uint64_t yearOfEra =
        (dayOfEra - dayOfEra / 1460 + dayOfEra / 36524 - dayOfEra / 146096) / 365;
    const std::uint64_t dayOfYear = dayOfEra - (365 * yearOfEra + yearOfEra / 4 - yearOfEra / 100);
    const std::uint64_t monthIndex = (5 * dayOfYear + 2) / 153;
    const std::uint64_t day = dayOfYear - (153 * monthIndex + 2) / 5 + 1;
    const std::uint64_t month = monthIndex < 10 ? monthIndex + 3 : monthIndex - 9;
    const std::int64_t year =
        static_cast<std::int64_t>(yearOfEra) + era * 400 + (month <= 2 ? 1 : 0);

    appendNumber(static_cast<std::uint64_t>(std::max<std::int64_t>(year, 0)), 4);
    append("-");
    appendNumber(month, 2);
    append("-");
    appendNumber(day, 2);
    append(std::string_view(&separator, 1));
    appendNumber(static_cast<std::uint64_t>(time / 3600), 2);
    append(":");
    appendNumber(static_cast<std::uint64_t>(time / 60 % 60), 2);
    append(":");
    appendNumber(static_cast<std::uint64_t>(time % 60), 2);
  }

  [[nodiscard]] auto data() const -> const char* { return buffer.data(); }
  [[nodiscard]] auto length() const -> std::size_t { return size; }
  [[nodiscard]] auto view() const -> std::string_view { return {buffer.data(), size}; }

  private:
  std::array<char, Capacity> buffer{};
  std::size_t size{0};
};

/** What the signal handler uses, set up before it is installed */
struct SignalState {
  std::atomic<LogSignalHandler> handler{nullptr};
  /** Whether a signal is being reported; it stays set, as the program ends after the report */
  std::atomic<bool> handling{false};
  /**
   * The fields set with Logger::setField(), as the members of a JSON object, each after a comma;
   * replaced rather than changed. One that a signal handler may read is never deleted.
   */
  std::atomic<const std::string*> context{nullptr};
  // the parts of the report, in memory of their own, as the handler may run on a small stack
  FixedText<1024> message;
  FixedText<1024> messageTemplate;
  FixedText<512> arguments;
  FixedText<256> fields;
  FixedText<2048> json;
  FixedText<4096> line;
  std::array<void*, 256> frames{};
};

inline auto signalState() -> SignalState& {
  // never destroyed, as a signal may come at any time
  static auto* const State = new SignalState();
  return *State;
}

#ifdef UTILS_CRASH_SIGNALS

/** The name of a signal, and what it means */
struct SignalName {
  const char* name;
  const char* meaning;
};

inline auto signalName(int number) -> SignalName {
  switch (number) {
  case SIGSEGV:
    return {"SIGSEGV", "segmentation fault"};
  case SIGBUS:
    return {"SIGBUS", "bus error"};
  case SIGFPE:
    return {"SIGFPE", "arithmetic error"};
  case SIGILL:
    return {"SIGILL", "illegal instruction"};
  case SIGABRT:
    return {"SIGABRT", "abort"};
  default:
    return {"unknown", "unknown signal"};
  }
}

/** What the code of a signal the system raised for a fault says; nullptr for none known */
inline auto signalCause(int number, int code) -> const char* {
  switch (number) {
  case SIGSEGV:
    switch (code) {
    case SEGV_MAPERR:
      return "address not mapped";
    case SEGV_ACCERR:
      return "no permission for the address";
    default:
      return nullptr;
    }
  case SIGBUS:
    switch (code) {
    case BUS_ADRALN:
      return "misaligned address";
    case BUS_ADRERR:
      return "address does not exist";
    case BUS_OBJERR:
      return "hardware error";
    default:
      return nullptr;
    }
  case SIGFPE:
    switch (code) {
    case FPE_INTDIV:
      return "integer division by zero";
    case FPE_INTOVF:
      return "integer overflow";
    case FPE_FLTDIV:
      return "floating-point division by zero";
    case FPE_FLTOVF:
      return "floating-point overflow";
    case FPE_FLTUND:
      return "floating-point underflow";
    case FPE_FLTRES:
      return "inexact floating-point result";
    case FPE_FLTINV:
      return "invalid floating-point operation";
    case FPE_FLTSUB:
      return "subscript out of range";
    default:
      return nullptr;
    }
  case SIGILL:
    switch (code) {
    case ILL_ILLOPC:
      return "illegal opcode";
    case ILL_ILLOPN:
      return "illegal operand";
    case ILL_ILLADR:
      return "illegal addressing mode";
    case ILL_ILLTRP:
      return "illegal trap";
    case ILL_PRVOPC:
      return "privileged opcode";
    case ILL_PRVREG:
      return "privileged register";
    case ILL_COPROC:
      return "coprocessor error";
    case ILL_BADSTK:
      return "internal stack error";
    default:
      return nullptr;
    }
  default:
    return nullptr;
  }
}

/** Writes all of text to the file descriptor, as far as it goes */
inline void writeAll(int descriptor, std::string_view text) {
  while (!text.empty()) {
    const auto written = ::write(descriptor, text.data(), text.size());
    if (written < 0 && errno == EINTR) {
      continue;
    }
    if (written <= 0) {
      return;
    }
    text.remove_prefix(static_cast<std::size_t>(written));
  }
}

/**
 * Renders the fields of setField() for the JSON line of a signal; called under the lock whenever
 * they change
 */
inline void renderContext(LoggerState& state) {
  auto text = std::make_unique<std::string>();
  for (const auto& [name, value] : *state.context) {
    *text += ',';
    appendJsonString(*text, name);
    *text += ':';
    appendJsonValue(*text, value);
  }
  auto& signals = signalState();
  const std::string* previous = signals.context.exchange(text.release());
  // a signal handler that runs may be reading the one before; then it is kept
  if (!signals.handling.load()) {
    delete previous;
  }
}

/** Ends the program by the signal, as its default action does */
[[noreturn]] inline void endBySignal(int number) {
  struct sigaction action{};
  action.sa_handler = SIG_DFL;
  sigemptyset(&action.sa_mask);
  sigaction(number, &action, nullptr);
  // the signal is blocked while its handler runs
  sigset_t unblock;
  sigemptyset(&unblock);
  sigaddset(&unblock, number);
  pthread_sigmask(SIG_UNBLOCK, &unblock, nullptr);
  static_cast<void>(raise(number));
  // the default action of the signals caught ends the process; in case it did not
  _exit(128 + number);
}

/** Puts together the report of a signal: the message, its template and values, and the JSON */
inline void describeSignal(int number, const siginfo_t* info, const LogTime& time) {
  auto& signals = signalState();
  const auto [name, meaning] = signalName(number);
  // a code above 0 means that the system raised the signal, for a fault (not for SIGABRT)
  const bool fault = info != nullptr && info->si_code > 0 && number != SIGABRT;
  const char* cause = fault ? signalCause(number, info->si_code) : nullptr;
  const bool sent = info != nullptr && info->si_code == SI_USER && info->si_pid != getpid();
  // where the fault was: the address accessed, or the instruction that failed
  const bool memory = number == SIGSEGV || number == SIGBUS;

  FixedText<64> where;
  if (fault) {
    where.appendHex(reinterpret_cast<std::uintptr_t>(info->si_addr));
  } else if (sent) {
    where.appendNumber(static_cast<std::uint64_t>(info->si_pid));
  }
  FixedText<128> what;
  what.append(meaning);
  if (cause != nullptr) {
    what.append(": ");
    what.append(cause);
  }

  auto& message = signals.message;
  auto& messageTemplate = signals.messageTemplate;
  message.clear();
  messageTemplate.clear();
  message.append("The process received the signal ");
  messageTemplate.append("The process received the signal ");
  message.append(name);
  message.append(" (");
  message.append(what.view());
  message.append(")");
  messageTemplate.append("{} ({})");
  const char* place = nullptr;
  if (fault) {
    place = memory ? ", for the address " : ", at the instruction ";
  } else if (sent) {
    place = ", sent by the process ";
  }
  if (place != nullptr) {
    message.append(place);
    message.append(where.view());
    messageTemplate.append(place);
    messageTemplate.append("{}");
  }
  message.append(".");
  messageTemplate.append(".");

  // the values; none of the texts has a character that JSON escapes
  auto& arguments = signals.arguments;
  auto& fields = signals.fields;
  arguments.clear();
  fields.clear();
  arguments.append("[\"");
  arguments.append(name);
  arguments.append("\",\"");
  arguments.append(what.view());
  arguments.append("\"");
  fields.append(R"({"signal":")");
  fields.append(name);
  fields.append("\"");
  if (fault) {
    arguments.append(",\"");
    arguments.append(where.view());
    arguments.append("\"");
    fields.append(R"(,"address":")");
    fields.append(where.view());
    fields.append("\"");
  } else if (sent) {
    arguments.append(",");
    arguments.append(where.view());
    fields.append(R"(,"sender":)");
    fields.append(where.view());
  }
  arguments.append("]");
  fields.append("}");

  FixedText<64> iso;
  iso.appendCalendar(time.seconds, 'T');
  iso.append(".");
  iso.appendNumber(static_cast<std::uint64_t>(time.nanoseconds / 1000), 6);
  iso.append("Z");

  auto& json = signals.json;
  json.clear();
  json.append(R"({"message":")");
  json.append(message.view());
  json.append("\",");
  // the members of the fields, without their braces
  json.append(fields.view().substr(1, fields.length() - 2));
  json.append(R"(,"time":")");
  json.append(iso.view());
  json.append("\"}");
}

/** Writes the report of a signal to stderr, and to the JSON Lines file if there is one */
inline void reportSignal(const LogTime& time) {
  auto& state = loggerState();
  auto& signals = signalState();
  const int rank = state.rank.load(std::memory_order_relaxed);

  // the line of the console, as for an error, in local time
  auto& line = signals.line;
  line.clear();
  line.appendCalendar(time.seconds + state.utcOffset.load(std::memory_order_relaxed), ' ');
  line.append(".");
  line.appendNumber(static_cast<std::uint64_t>(time.nanoseconds / 1000000), 3);
  line.append(" error ");
  if (rank >= 0) {
    line.appendNumber(static_cast<std::uint64_t>(rank));
  } else {
    line.append("-");
  }
  line.append(" : ");
  line.append(signals.message.view());
  line.append(" \n");
  writeAll(STDERR_FILENO, line.view());

#ifdef UTILS_LOGGER_HAS_BACKTRACE
  const int depth = std::min(state.backtraceDepth.load(std::memory_order_relaxed),
                             static_cast<int>(signals.frames.size()));
  if (depth > 0) {
    // backtrace() was called once before, so that it does not load the unwinder (and allocate) now
    const int count = backtrace(signals.frames.data(), depth);
    writeAll(STDERR_FILENO, "Backtrace:\n");
    backtrace_symbols_fd(signals.frames.data(), count, STDERR_FILENO);
  }
#endif

  const int descriptor = state.jsonDescriptor.load();
  if (descriptor >= 0) {
    line.clear();
    if (state.jsonWriting.load()) {
      // ends the line that the signal broke off
      line.append("\n");
    }
    FixedText<64> iso;
    iso.appendCalendar(time.seconds, 'T');
    iso.append(".");
    iso.appendNumber(static_cast<std::uint64_t>(time.nanoseconds / 1000), 6);
    iso.append("Z");
    line.append(R"({"time":")");
    line.append(iso.view());
    line.append(R"(","level":"error","rank":)");
    if (rank >= 0) {
      line.appendNumber(static_cast<std::uint64_t>(rank));
    } else {
      line.append("null");
    }
    if (const std::string* context = signals.context.load(); context != nullptr) {
      line.append(*context);
    }
    line.append(R"(,"message":")");
    line.append(signals.message.view());
    line.append(R"(","template":")");
    line.append(signals.messageTemplate.view());
    line.append(R"(","arguments":)");
    line.append(signals.arguments.view());
    line.append(R"(,"fields":)");
    line.append(signals.fields.view());
    line.append(R"(,"source":{"file":"","line":0,"function":""}})");
    line.append("\n");
    writeAll(descriptor, line.view());
  }
}

/** Writes out what stdout holds, unless another thread is writing to it */
inline void flushOutput() {
  // (not async-signal-safe by the letter; trylock keeps it from waiting for another thread)
  if (ftrylockfile(stdout) == 0) {
    static_cast<void>(std::fflush(stdout));
    funlockfile(stdout);
  }
}

inline void onSignal(int number, siginfo_t* info, void* /*context*/) {
  auto& signals = signalState();
  const bool ending = loggerState().ending.load();
  if (signals.handling.exchange(true)) {
    // abort() while a signal is reported, e.g. from MPI_Abort() in the handler: that ends it
    if (number == SIGABRT) {
      endBySignal(number);
    }
    // a fault of another thread at the same time: the program ends with the first report
    for (;;) {
      pause();
    }
  }
  // the abort() at the end of a fatal error, which was reported already
  if (number == SIGABRT && ending) {
    endBySignal(number);
  }

  timespec now{};
  clock_gettime(CLOCK_REALTIME, &now);
  const LogTime time{static_cast<std::int64_t>(now.tv_sec), static_cast<std::int32_t>(now.tv_nsec)};
  describeSignal(number, info, time);
  flushOutput();
  reportSignal(time);

  const auto handler = signals.handler.load();
  if (handler != nullptr) {
    LogSignal signal;
    signal.number = number;
    signal.name = signalName(number).name;
    signal.message = signals.message.data();
    signal.json = signals.json.data();
    handler(signal);
  }
  endBySignal(number);
}

/** Sets up what the signal handler needs, so that it need not allocate */
inline void prepareSignals() {
  // the unwinder is loaded at the first call of backtrace()
#ifdef UTILS_LOGGER_HAS_BACKTRACE
  auto& frames = signalState().frames;
  static_cast<void>(backtrace(frames.data(), 1));
#endif

  // a stack for the handler, so that it runs after a stack overflow of this thread as well
  stack_t current{};
  if (sigaltstack(nullptr, &current) == 0 && (current.ss_flags & SS_DISABLE) != 0) {
    const auto size = std::max<std::size_t>(64 * 1024, static_cast<std::size_t>(SIGSTKSZ));
    stack_t stack{};
    // kept for as long as the thread runs
    stack.ss_sp = new char[size];
    stack.ss_size = size;
    stack.ss_flags = 0;
    sigaltstack(&stack, nullptr);
  }

  // the time zone, until a message on the console updates it
  const std::time_t time = std::time(nullptr);
  std::tm calendar{};
  localtime_r(&time, &calendar);
  auto& state = loggerState();
  state.utcOffset.store(calendar.tm_gmtoff, std::memory_order_relaxed);

  const std::lock_guard<std::recursive_mutex> lock(state.mutex);
  state.contextChanged = &renderContext;
  renderContext(state);
}

#endif // UTILS_CRASH_SIGNALS

/** The type of the exception being handled, as written in the code where that can be told */
inline auto currentExceptionType() -> std::string {
#ifdef UTILS_CRASH_DEMANGLE
  const std::type_info* type = abi::__cxa_current_exception_type();
  if (type == nullptr) {
    return {};
  }
  int status = 0;
  char* readable = abi::__cxa_demangle(type->name(), nullptr, nullptr, &status);
  std::string name = status == 0 && readable != nullptr ? readable : type->name();
  std::free(static_cast<void*>(readable));
  return name;
#else
  return {};
#endif
}

/** The message of an exception derived from std::exception; empty for others */
inline auto exceptionMessage(const std::exception_ptr& exception) -> std::string {
  try {
    std::rethrow_exception(exception);
  } catch (const std::exception& error) {
    return error.what();
  } catch (...) {
    // neither a message nor anything else to tell
  }
  return {};
}

[[noreturn]] inline void onTerminate() noexcept {
  try {
    const auto exception = std::current_exception();
    if (exception == nullptr) {
      FatalLogger(true) << "std::terminate() was called without an exception.";
    }
    const auto what = exceptionMessage(exception);
    const auto type = currentExceptionType();
    if (type.empty()) {
      FatalLogger(true) << "An exception was not caught:" << what;
    }
    if (what.empty()) {
      FatalLogger(true) << "An exception of type" << type << "was not caught.";
    }
    FatalLogger(true) << "An exception of type" << type << "was not caught:" << what;
  } catch (...) {
    // the program ends below in any case
  }
  std::abort();
}

} // namespace detail

/**
 * Reports a fatal signal the way the logger reports a fatal error: SIGSEGV, SIGBUS, SIGFPE, SIGILL
 * and SIGABRT
 *
 * The report is a line on stderr in the format of the console (whether the console is enabled or
 * not), the backtrace, and a line in the JSON Lines file, if there is one. Then handler is called,
 * if there is one; after it, the signal ends the program as it does by default. SIGABRT from the
 * end of a fatal error is not reported again. Callbacks get no message: they cannot be called from
 * a signal handler.
 *
 * A signal that has a handler already keeps it, e.g. one of a sanitizer. The MPI library may set
 * handlers when it is initialized, for the signals that have none yet; so call this before. The
 * handler runs on a stack of its own on the calling thread, so that it can report a stack overflow
 * there; on other threads, it runs on their stack.
 *
 * Calling it again sets the handler only. Without POSIX, it does nothing.
 */
inline void catchSignals(LogSignalHandler handler = nullptr) {
#ifdef UTILS_CRASH_SIGNALS
  auto& signals = detail::signalState();
  signals.handler.store(handler);
  detail::prepareSignals();

  struct sigaction action{};
  action.sa_sigaction = &detail::onSignal;
  action.sa_flags = SA_SIGINFO | SA_ONSTACK;
  // one report at a time per thread
  sigemptyset(&action.sa_mask);
  for (const int number : detail::CrashSignals) {
    sigaddset(&action.sa_mask, number);
  }
  for (const int number : detail::CrashSignals) {
    struct sigaction current{};
    if (sigaction(number, nullptr, &current) != 0) {
      continue;
    }
    const bool unset = (current.sa_flags & SA_SIGINFO) == 0 && current.sa_handler == SIG_DFL;
    if (unset) {
      sigaction(number, &action, nullptr);
    }
  }
#else
  static_cast<void>(handler);
#endif
}

/**
 * Ends the program on an exception that nothing caught as on a fatal error: logError() gives its
 * type and message, and the abort handler (see Logger::setAbortHandler()) ends the program.
 *
 * The backtrace usually reaches back to where the exception was thrown, as the stack is not unwound
 * for an exception that nothing catches (with Clang, an exception that leaves a noexcept function
 * unwinds the stack up to that function).
 */
inline void catchUncaughtExceptions() { std::set_terminate(&detail::onTerminate); }

} // namespace logger_v3
} // namespace utils

#endif // UTILS_CRASH_H_
