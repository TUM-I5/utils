// SPDX-FileCopyrightText: 2013-2026 Technical University of Munich
//
// SPDX-License-Identifier: BSD-3-Clause
//
// SPDX-FileContributor: Sebastian Rettenberger
// SPDX-FileContributor: David Schneller

#ifndef UTILS_LOGGER_H_
#define UTILS_LOGGER_H_

#include "utils/common.h"
#include "utils/stringutils.h"
#include "utils/timeutils.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <mutex>
#include <ostream>
#include <signal.h>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

// backtraces are only available where execinfo.h is (e.g. glibc and macOS, but not Windows)
#if __has_include(<execinfo.h>)
#include <execinfo.h>
#define UTILS_LOGGER_HAS_BACKTRACE 1
#endif

#ifndef LOG_LEVEL
#ifdef NDEBUG
#define LOG_LEVEL 2
#else // NDEBUG
#define LOG_LEVEL 3
#endif // NDEBUG
#endif // LOG_LEVEL

#ifdef LOG_ABORT
#error                                                                                             \
    "LOG_ABORT is not used anymore: set what ends the program with utils::Logger::setAbortHandler()"
#endif

#ifdef BACKTRACE_SIZE
#error "BACKTRACE_SIZE is not used anymore: set the depth with utils::Logger::setBacktraceDepth()"
#endif

// whether the compiler has builtins that give the place of a call when used as default arguments
#if defined(__has_builtin)
#if __has_builtin(__builtin_FILE) && __has_builtin(__builtin_LINE) &&                              \
    __has_builtin(__builtin_FUNCTION)
#define UTILS_LOGGER_LOCATION_BUILTINS 1
#endif
#elif defined(__GNUC__) && !defined(__clang__) && !defined(__NVCOMPILER) &&                        \
    !defined(__INTEL_COMPILER)
#define UTILS_LOGGER_LOCATION_BUILTINS 1
#elif defined(_MSC_VER) && _MSC_VER >= 1926
#define UTILS_LOGGER_LOCATION_BUILTINS 1
#endif

#if !defined(UTILS_LOGGER_LOCATION_BUILTINS) && __has_include(<source_location>)
#include <source_location>
#endif

#define UTILS_LOGGER_CONCAT_IMPL(a, b) a##b
#define UTILS_LOGGER_CONCAT(a, b) UTILS_LOGGER_CONCAT_IMPL(a, b)
#define UTILS_LOGGER_LEVEL_NAMESPACE UTILS_LOGGER_CONCAT(utils_logger_v3_level, LOG_LEVEL)

/**
 * A collection of useful utility functions
 */
namespace utils {

/**
 * The logger's types and functions live in an inline namespace named after the version of their
 * layout. A program can load libraries that embed different versions of this header; each version
 * then has symbols of its own, rather than one version's code running on another's objects.
 */
inline namespace logger_v3 {

/** How severe a message is */
enum class LogLevel {
  /** Details for debugging */
  Debug,
  /** Information on the progress (printed to stdout) */
  Info,
  /** Something that may be wrong */
  Warning,
  /** An error; from logError() and logFatal(), a fatal one, which ends the program */
  Error
};

/** When a message was made: seconds and nanoseconds since the Unix epoch */
struct LogTime {
  std::int64_t seconds{0};
  std::int32_t nanoseconds{0};
};

/** A value of a message, as structured output gets it */
struct LogValue {
  using List = std::vector<LogValue>;
  /** Nothing (std::monostate) for values that have no form here, e.g. a null pointer */
  std::variant<std::monostate, bool, std::int64_t, std::uint64_t, double, std::string, List> value;
};

/** A named value of a message */
using LogNamedValue = std::pair<std::string, LogValue>;

/** Where in the source code a message is made */
struct LogLocation {
  const char* file{""};
  const char* function{""};
  std::uint32_t line{0};

  /**
   * The place of the call, when used as a default argument; empty where that cannot be told
   *
   * The builtins come first where there are any, so that the function is its plain name both in
   * C++17 and in C++20.
   */
#if defined(UTILS_LOGGER_LOCATION_BUILTINS)
  static constexpr auto current(const char* fileName = __builtin_FILE(),
                                const char* functionName = __builtin_FUNCTION(),
                                std::uint32_t lineNumber = __builtin_LINE()) noexcept
      -> LogLocation {
    return {fileName, functionName, lineNumber};
  }
#elif defined(__cpp_lib_source_location)
  static constexpr auto
      current(std::source_location place = std::source_location::current()) noexcept
      -> LogLocation {
    return {place.file_name(), place.function_name(), place.line()};
  }
#else
  static constexpr auto current() noexcept -> LogLocation { return {}; }
#endif
};

/** A message, as the logger hands it on */
struct LogRecord {
  LogLevel level{LogLevel::Info};
  LogTime time;
  /** The rank that made the message; -1 if that was before the rank was known */
  int rank{-1};
  /** Whether every rank shows the message, and not only the display rank */
  bool broadcast{false};
  /** The message, as the console shows it after the time stamp, the level and the rank */
  std::string text;
  /**
   * The message with each value replaced by {}, and each brace of its text doubled; the same for
   * all messages of a statement, as long as they put together the same kinds of values
   */
  std::string messageTemplate;
  /** The values, in the order of the {} */
  std::vector<LogValue> arguments;
  /** The values named with utils::field() */
  std::vector<LogNamedValue> fields;
  /** The fields set with Logger::setField(), the same for all messages */
  std::vector<LogNamedValue> context;
  LogLocation location;

  /** The message without the spaces it ends with */
  [[nodiscard]] auto message() const -> std::string_view {
    const auto end = text.find_last_not_of(' ');
    return std::string_view(text).substr(0, end == std::string::npos ? 0 : end + 1);
  }
};

/** A value that structured output gets under a name; see field() */
template <typename T>
struct LogField {
  std::string_view name;
  const T& value;
};

/**
 * Names a value of a message for structured output; the console shows the value as usual
 *
 * Example: <code>logInfo() << "The mesh has" << utils::field("cells", cells) << "cells.";</code>
 */
template <typename T>
auto field(std::string_view name, const T& value) -> LogField<T> {
  return {name, value};
}

/** Whether a fatal error raises SIGTRAP before it ends the program, to stop in a debugger */
enum class LogTrap {
  /** Never; the default */
  Never,
  /**
   * If a tracer is attached to the process, as a debugger is; that can only be told on Linux.
   * A tracer that is no debugger, such as strace, lets SIGTRAP end the process.
   */
  IfDebugger,
  /** Always; without a debugger, SIGTRAP ends the process */
  Always
};

/**
 * What ends the program after a fatal error, given the record of the error
 *
 * It must not return; if it does, the program ends with std::abort().
 */
using LogAbortHandler = std::function<void(const LogRecord&)>;

/**
 * Gets every message of this process from the level it is registered with on, whatever rank shows
 * it, e.g. to hand it on to another logging framework
 *
 * Callbacks are called one at a time, from whichever thread made the message, and they must copy
 * what they keep of the record. A message that a callback makes itself goes to the console only.
 */
using LogCallback = std::function<void(const LogRecord&)>;

namespace detail {

struct CallbackEntry {
  int id;
  LogLevel minimum;
  LogCallback callback;
};

using CallbackList = std::vector<CallbackEntry>;

/** What all messages share: the settings of the logger, and the lock for handing messages on */
struct LoggerState {
  std::atomic<int> rank{-1};
  std::atomic<int> displayRank{0};
  std::atomic<bool> logAll{false};
  std::atomic<LogLevel> minimumLevel{LogLevel::Debug};
  std::atomic<LogTrap> trap{LogTrap::Never};
  std::atomic<int> backtraceDepth{50};
  std::atomic<bool> console{true};
  std::atomic<bool> anyCallback{false};
  /** Recursive, so that callbacks may change the settings */
  std::recursive_mutex mutex;
  /** Guarded by the mutex */
  LogAbortHandler abortHandler;
  /** Guarded by the mutex; replaced rather than changed, so that a dispatch can go on with its copy
   */
  std::shared_ptr<const CallbackList> callbacks{std::make_shared<const CallbackList>()};
  /** Guarded by the mutex */
  int nextCallbackId{1};
  /** Guarded by the mutex */
  std::vector<LogNamedValue> context;
};

inline auto loggerState() -> LoggerState& {
  // never destroyed, so that messages from destructors of static objects still find it
  static auto* const State = new LoggerState();
  return *State;
}

/** Whether this thread is handing on a message right now */
inline auto dispatching() -> bool& {
  thread_local bool flag = false;
  return flag;
}

/** Marks this thread as handing on a message for as long as it exists */
class DispatchScope {
  public:
  DispatchScope() { dispatching() = true; }
  ~DispatchScope() { dispatching() = false; }
  DispatchScope(const DispatchScope&) = delete;
  DispatchScope(DispatchScope&&) = delete;
  auto operator=(const DispatchScope&) -> DispatchScope& = delete;
  auto operator=(DispatchScope&&) -> DispatchScope& = delete;
};

template <typename T>
struct IsCharacter : std::bool_constant<std::is_same_v<T, char> || std::is_same_v<T, signed char> ||
                                        std::is_same_v<T, unsigned char>> {};

template <typename T>
auto toLogValue(const T& data) -> LogValue;

template <typename T, std::size_t... Idx>
auto tupleToLogValue(const T& data, std::index_sequence<Idx...> /*indices*/) -> LogValue {
  LogValue::List list;
  list.reserve(sizeof...(Idx));
  (list.push_back(toLogValue(std::get<Idx>(data))), ...);
  return LogValue{std::move(list)};
}

/** The value of data for structured output; types are taken in the same order as for the console */
template <typename T>
auto toLogValue(const T& data) -> LogValue {
  using Decayed = std::decay_t<T>;
  if constexpr (std::is_same_v<T, bool>) {
    return LogValue{data};
  } else if constexpr (IsCharacter<T>::value) {
    return LogValue{std::string(1, static_cast<char>(data))};
  } else if constexpr (std::is_integral_v<T> && std::is_signed_v<T>) {
    return LogValue{static_cast<std::int64_t>(data)};
  } else if constexpr (std::is_integral_v<T>) {
    return LogValue{static_cast<std::uint64_t>(data)};
  } else if constexpr (std::is_floating_point_v<T>) {
    return LogValue{static_cast<double>(data)};
  } else if constexpr (std::is_same_v<T, std::nullptr_t>) {
    return LogValue{};
  } else if constexpr (std::is_same_v<Decayed, const char*> || std::is_same_v<Decayed, char*>) {
    const char* text = data;
    return text == nullptr ? LogValue{} : LogValue{std::string(text)};
  } else if constexpr (std::is_convertible_v<const T&, std::string_view>) {
    return LogValue{std::string(std::string_view(data))};
  } else if constexpr (CanOutput<T>::Value) {
    std::ostringstream stream;
    stream << data;
    return LogValue{stream.str()};
  } else if constexpr (IsIterable<T>::Value) {
    LogValue::List list;
    for (const auto& element : data) {
      list.push_back(toLogValue(element));
    }
    return LogValue{std::move(list)};
  } else if constexpr (IsGettable<T>::Value) {
    return tupleToLogValue(data, std::make_index_sequence<std::tuple_size_v<T>>{});
  } else {
    return LogValue{};
  }
}

/** Appends text, with each brace doubled, as a message template has it */
inline void appendTemplateText(std::string& messageTemplate, std::string_view text) {
  for (const char character : text) {
    messageTemplate += character;
    if (character == '{' || character == '}') {
      messageTemplate += character;
    }
  }
}

inline auto now() -> LogTime {
  const auto sinceEpoch = std::chrono::system_clock::now().time_since_epoch();
  const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(sinceEpoch);
  const auto nanoseconds =
      std::chrono::duration_cast<std::chrono::nanoseconds>(sinceEpoch - seconds);
  return {static_cast<std::int64_t>(seconds.count()),
          static_cast<std::int32_t>(nanoseconds.count())};
}

inline auto levelName(LogLevel level) -> const char* {
  switch (level) {
  case LogLevel::Debug:
    return "debug";
  case LogLevel::Info:
    return "info";
  case LogLevel::Warning:
    return "warn";
  case LogLevel::Error:
    return "error";
  }
  return "unknown";
}

/** Whether the console of this rank shows the message */
inline auto shownOnConsole(const LogRecord& record) -> bool {
  const auto& state = loggerState();
  return record.rank == state.displayRank.load(std::memory_order_relaxed) || record.rank == -1 ||
         state.logAll.load(std::memory_order_relaxed) || record.broadcast;
}

inline void writeToConsole(const LogRecord& record) {
  // (spelled out, since not every C runtime understands %F or %T; e.g. msvcrt.dll does not)
  std::string line =
      TimeUtils::timeAsString("%Y-%m-%d %H:%M:%S", static_cast<time_t>(record.time.seconds));
  line += ".";
  line += StringUtils::padLeft(std::to_string(record.time.nanoseconds / 1000000), 3, '0');
  line += " ";
  line += levelName(record.level);
  line += " ";
  line += record.rank >= 0 ? std::to_string(record.rank) : std::string("-");
  line += " : ";
  line += record.text;
  line += '\n';

  if (record.level == LogLevel::Info || record.level == LogLevel::Debug) {
    std::cout << line;
  } else {
    std::cerr << line;
  }
}

/** Whether a message goes anywhere at all; if not, it need not be put together */
inline auto wanted(const LogRecord& record) -> bool {
  const auto& state = loggerState();
  if (record.level == LogLevel::Error) {
    return true;
  }
  if (record.level < state.minimumLevel.load(std::memory_order_relaxed)) {
    return false;
  }
  return state.anyCallback.load(std::memory_order_relaxed) ||
         (state.console.load(std::memory_order_relaxed) && shownOnConsole(record));
}

/** Hands a finished message on to where messages go */
inline void dispatch(LogRecord& record) noexcept {
  try {
    auto& state = loggerState();
    if (dispatching()) {
      // made while handing on another message: write it right away, without waiting for the lock
      if (shownOnConsole(record)) {
        writeToConsole(record);
      }
      return;
    }
    const std::lock_guard<std::recursive_mutex> lock(state.mutex);
    const DispatchScope scope;
    record.context = state.context;
    if (state.console.load(std::memory_order_relaxed) && shownOnConsole(record)) {
      writeToConsole(record);
    }
    const auto callbacks = state.callbacks;
    for (const auto& entry : *callbacks) {
      if (record.level >= entry.minimum) {
        try {
          entry.callback(record);
        } catch (...) {
          // a callback that fails does not keep the others from the message
        }
      }
    }
  } catch (...) {
    // a message that cannot be handed on is lost; the program goes on
  }
}

/** Whether a debugger is attached to the process; false where that cannot be told */
inline auto debuggerAttached() -> bool {
#ifdef __linux__
  std::FILE* status = std::fopen("/proc/self/status", "r");
  if (status == nullptr) {
    return false;
  }
  constexpr std::string_view Key = "TracerPid:";
  char line[256];
  bool attached = false;
  while (std::fgets(line, sizeof(line), status) != nullptr) {
    if (std::strncmp(line, Key.data(), Key.size()) == 0) {
      attached = std::strtol(line + Key.size(), nullptr, 10) != 0;
      break;
    }
  }
  static_cast<void>(std::fclose(status));
  return attached;
#else
  return false;
#endif
}

inline void writeBacktrace(int depth) {
#ifdef UTILS_LOGGER_HAS_BACKTRACE
  if (depth <= 0) {
    return;
  }
  std::vector<void*> frames(static_cast<std::size_t>(depth));
  const int count = backtrace(frames.data(), depth);
  char** names = backtrace_symbols(frames.data(), count);

  // Buffer output to avoid interlacing with other processes
  std::string text = "Backtrace:\n";
  for (int i = 0; i < count; ++i) {
    text += names != nullptr ? names[i] : "?";
    text += '\n';
  }
  std::free(static_cast<void*>(names));

  std::cerr << text << std::flush;
#else
  static_cast<void>(depth);
#endif
}

/** Whether this thread is ending the program because of a fatal error right now */
inline auto aborting() -> bool& {
  thread_local bool flag = false;
  return flag;
}

/**
 * Ends the program after the fatal error of record
 *
 * In order: the output is flushed, the backtrace written, SIGTRAP raised if set so, and the abort
 * handler called; if there is none, or it returns, std::abort() ends the program.
 */
[[noreturn]] inline void endProgram(const LogRecord& record) noexcept {
  auto& state = loggerState();
  if (!aborting()) {
    aborting() = true;
    try {
      std::cout.flush();
      std::cerr.flush();
      writeBacktrace(state.backtraceDepth.load(std::memory_order_relaxed));

      const auto trap = state.trap.load(std::memory_order_relaxed);
      // Windows has no SIGTRAP; and raising an unknown signal there would end up in the
      // invalid parameter handler of the C runtime instead
#if defined(SIGTRAP) && !defined(_WIN32)
      if (trap == LogTrap::Always || (trap == LogTrap::IfDebugger && debuggerAttached())) {
        static_cast<void>(std::raise(SIGTRAP));
      }
#else
      static_cast<void>(trap);
#endif

      LogAbortHandler handler;
      {
        const std::lock_guard<std::recursive_mutex> lock(state.mutex);
        handler = state.abortHandler;
      }
      if (handler) {
        handler(record);
      }
    } catch (...) {
      // the program ends below in any case
    }
  }
  // a fatal error in the abort handler itself ends up here directly
  std::abort();
}

} // namespace detail

/**
 * A message being put together with operator<<
 *
 * Most of the code is taken from QDebug form the Qt Framework
 */
class LogMessage {
  public:
  LogMessage(const LogMessage&) = delete;
  LogMessage(LogMessage&&) = delete;
  auto operator=(const LogMessage&) -> LogMessage& = delete;
  auto operator=(LogMessage&&) -> LogMessage& = delete;

  /********* Space handling *********/

  /**
   * Add a space to output message and activate spaces
   */
  auto space() -> LogMessage& {
    spacing = true;
    buffer << ' ';
    templateText(" ");
    return *this;
  }
  /**
   * Deactivate spaces
   */
  auto nospace() -> LogMessage& {
    spacing = false;
    return *this;
  }
  /**
   * Add space of activated
   */
  auto maybeSpace() -> LogMessage& {
    if (spacing) {
      buffer << ' ';
      templateText(" ");
    }
    return *this;
  }

  /**
   * Default function to add messages; the message template gets {} for the value
   */
  template <typename T>
  auto operator<<(const T& data) -> LogMessage& {
    if (!active) {
      return *this;
    }
    return appendValue(data);
  }

  /**
   * A string literal (or another constant char array), which is text of the message template
   */
  template <std::size_t N>
  auto operator<<(const char (&text)[N]) -> LogMessage& {
    if (!active) {
      return *this;
    }
    const std::string_view view(text);
    buffer << view;
    templateText(view);
    return maybeSpace();
  }

  /**
   * A char array that can change, which is a value
   */
  template <std::size_t N>
  auto operator<<(char (&text)[N]) -> LogMessage& {
    if (!active) {
      return *this;
    }
    return appendValue(text);
  }

  /**
   * A named value; the console shows the value as usual
   */
  template <typename T>
  auto operator<<(const LogField<T>& named) -> LogMessage& {
    if (!active) {
      return *this;
    }
    appendValue(named.value);
    if (nesting == 0) {
      record.fields.emplace_back(std::string(named.name), detail::toLogValue(named.value));
    }
    return *this;
  }

  /**
   * Operator to add functions like std::endl
   */
  auto operator<<(std::ostream& (*func)(std::ostream&)) -> LogMessage& {
    if (active) {
      const auto before = buffer.tellp();
      buffer << func;
      if (nesting == 0 && buffer.tellp() != before) {
        templateText(buffer.str().substr(static_cast<std::size_t>(before)));
      }
    }
    return *this; // No space in this case
  }

  protected:
  /**
   * Starts a message
   *
   * @param level How severe the message is
   * @param broadcast Whether every rank shows the message, and not only the display rank
   * @param location Where in the source code the message is made
   */
  LogMessage(LogLevel level, bool broadcast, const LogLocation& location)
      : record(startRecord(level, broadcast, location)), active(detail::wanted(record)) {}

  ~LogMessage() = default;

  /** Hands the message on, if it goes anywhere */
  void finish() noexcept {
    if (active) {
      try {
        record.text = buffer.str();
        const auto end = record.messageTemplate.find_last_not_of(' ');
        record.messageTemplate.resize(end == std::string::npos ? 0 : end + 1);
      } catch (...) {
        return;
      }
      detail::dispatch(record);
    }
  }

  [[nodiscard]] auto level() const -> LogLevel { return record.level; }

  /** The record of the message; complete after finish() */
  [[nodiscard]] auto finishedRecord() const -> const LogRecord& { return record; }

  private:
  /** Keeps the message template from parts of a value for as long as it exists */
  class Nesting {
public:
    explicit Nesting(LogMessage& outer) : owner(outer) { ++owner.nesting; }
    ~Nesting() { --owner.nesting; }
    Nesting(const Nesting&) = delete;
    Nesting(Nesting&&) = delete;
    auto operator=(const Nesting&) -> Nesting& = delete;
    auto operator=(Nesting&&) -> Nesting& = delete;

private:
    LogMessage& owner;
  };

  static auto startRecord(LogLevel level, bool broadcast, const LogLocation& location)
      -> LogRecord {
    LogRecord record;
    record.level = level;
    record.broadcast = broadcast;
    record.rank = detail::loggerState().rank.load(std::memory_order_relaxed);
    record.time = detail::now();
    record.location = location;
    return record;
  }

  void templateText(std::string_view text) {
    if (nesting == 0) {
      detail::appendTemplateText(record.messageTemplate, text);
    }
  }

  void templateValue(LogValue value) {
    if (nesting == 0) {
      record.messageTemplate += "{}";
      record.arguments.push_back(std::move(value));
    }
  }

  template <typename T>
  auto appendValue(const T& data) -> LogMessage& {
    if constexpr (std::is_invocable_r_v<LogMessage&, T, LogMessage&>) {
      return std::invoke(data, *this);
    } else if constexpr (std::is_same_v<T, std::string>) {
      buffer << '"' << data << '"';
      templateValue(detail::toLogValue(data));
      return maybeSpace();
    } else if constexpr (detail::IsCharacter<T>::value) {
      // characters are mostly punctuation; they are text of the message template
      buffer << data;
      const char character = static_cast<char>(data);
      templateText(std::string_view(&character, 1));
      return maybeSpace();
    } else if constexpr (CanOutput<T>::Value) {
      const auto before = buffer.tellp();
      buffer << data;
      // what puts out nothing, like std::setprecision(), is no value
      if (buffer.tellp() != before) {
        if constexpr (std::is_arithmetic_v<T> || std::is_pointer_v<std::decay_t<T>> ||
                      std::is_convertible_v<const T&, std::string_view>) {
          templateValue(detail::toLogValue(data));
        } else {
          templateValue(LogValue{buffer.str().substr(static_cast<std::size_t>(before))});
        }
      }
      return maybeSpace();
    } else if constexpr (IsIterable<T>::Value) {
      {
        const Nesting nested(*this);
        nospace() << '[';
        auto it = std::begin(data);
        if (it != std::end(data)) {
          *this << *it;
          ++it;
        }
        for (; it != std::end(data); ++it) {
          *this << ", " << *it;
        }
        *this << ']';
      }
      templateValue(detail::toLogValue(data));
      return space();
    } else if constexpr (IsGettable<T>::Value) {
      {
        const Nesting nested(*this);
        nospace() << '{';
        printTuple<T, 0>(*this, data);
        *this << '}';
      }
      templateValue(detail::toLogValue(data));
      return space();
    } else {
      // https://stackoverflow.com/questions/38304847/how-does-a-failed-static-assert-work-in-an-if-constexpr-false-block#comment119622305_64354296
      static_assert(sizeof(T) == 0, "Output for the given type not implemented.");
    }
  }

  template <typename T, std::size_t Idx>
  static void printTuple(LogMessage& message, const T& data) {
    if constexpr (Idx < std::tuple_size_v<T>) {
      if constexpr (Idx > 0) {
        message << ", ";
      }
      message << std::get<Idx>(data);
      printTuple<T, Idx + 1>(message, data);
    }
  }

  LogRecord record;
  /** Buffer for the output */
  std::stringstream buffer{std::stringstream::out};
  /** Print additional space */
  bool spacing{true};
  /** Whether the message goes anywhere; if not, it is not put together at all */
  bool active{true};
  /** How deep within a value the message is being put together */
  int nesting{0};
};

/**
 * Handles debugging/logging output
 *
 * A Logger is a message that is handed on when it is destroyed, i.e. at the end of the statement
 * that makes it. Its static functions set how the logger works.
 */
class Logger : public LogMessage {
  public:
  Logger(LogLevel level, bool broadcast, const LogLocation& location = LogLocation())
      : LogMessage(level, broadcast, location) {}

  Logger(const Logger&) = delete;
  Logger(Logger&&) = delete;
  auto operator=(const Logger&) -> Logger& = delete;
  auto operator=(Logger&&) -> Logger& = delete;

  ~Logger() { finish(); }

  using Trap = LogTrap;
  using AbortHandler = LogAbortHandler;

  /** Sets the rank that later messages are made by */
  static void setRank(int rank) {
    detail::loggerState().rank.store(rank, std::memory_order_relaxed);
  }
  /** Sets the rank whose console shows messages that are not broadcast */
  static void setDisplayRank(int rank) {
    detail::loggerState().displayRank.store(rank, std::memory_order_relaxed);
  }
  /** Sets whether the consoles of all ranks show all messages */
  static void setLogAll(bool logAll) {
    detail::loggerState().logAll.store(logAll, std::memory_order_relaxed);
  }
  /** Sets the least severe level shown; errors are always shown */
  static void setLevel(LogLevel minimum) {
    detail::loggerState().minimumLevel.store(minimum, std::memory_order_relaxed);
  }

  /**
   * Sets what ends the program after a fatal error, e.g. MPI_Abort() on the communicator the
   * program runs on. An empty handler leaves it to std::abort().
   *
   * @return The handler set before
   */
  static auto setAbortHandler(AbortHandler handler) -> AbortHandler {
    auto& state = detail::loggerState();
    const std::lock_guard<std::recursive_mutex> lock(state.mutex);
    std::swap(handler, state.abortHandler);
    return handler;
  }
  /** Sets whether a fatal error raises SIGTRAP first, to stop in a debugger; by default, never */
  static void setTrap(Trap trap) {
    detail::loggerState().trap.store(trap, std::memory_order_relaxed);
  }
  /** Sets how many frames the backtrace of a fatal error has; 0 for none */
  static void setBacktraceDepth(int depth) {
    detail::loggerState().backtraceDepth.store(depth, std::memory_order_relaxed);
  }

  using Callback = LogCallback;

  /**
   * Sets a field that all later messages carry, e.g. the id of the run, replacing one of the same
   * name
   */
  template <typename T>
  static void setField(const std::string& name, const T& value) {
    auto converted = detail::toLogValue(value);
    auto& state = detail::loggerState();
    const std::lock_guard<std::recursive_mutex> lock(state.mutex);
    for (auto& entry : state.context) {
      if (entry.first == name) {
        entry.second = std::move(converted);
        return;
      }
    }
    state.context.emplace_back(name, std::move(converted));
  }

  /** Removes the field set with setField() under name; nothing happens if there is none */
  static void removeField(const std::string& name) {
    auto& state = detail::loggerState();
    const std::lock_guard<std::recursive_mutex> lock(state.mutex);
    for (auto it = state.context.begin(); it != state.context.end(); ++it) {
      if (it->first == name) {
        state.context.erase(it);
        return;
      }
    }
  }

  /** Sets whether messages go to the console, e.g. not when another framework shows them */
  static void enableConsole(bool enable) {
    detail::loggerState().console.store(enable, std::memory_order_relaxed);
  }

  /**
   * Registers a callback, which gets every message of this process from minimum on
   *
   * @return The id to remove the callback with
   */
  static auto addCallback(Callback callback, LogLevel minimum = LogLevel::Debug) -> int {
    auto& state = detail::loggerState();
    const std::lock_guard<std::recursive_mutex> lock(state.mutex);
    auto callbacks = std::make_shared<detail::CallbackList>(*state.callbacks);
    const int id = state.nextCallbackId++;
    callbacks->push_back({id, minimum, std::move(callback)});
    state.callbacks = std::move(callbacks);
    state.anyCallback.store(true, std::memory_order_relaxed);
    return id;
  }

  /** Removes the callback registered with id; nothing happens if there is none */
  static void removeCallback(int id) {
    auto& state = detail::loggerState();
    const std::lock_guard<std::recursive_mutex> lock(state.mutex);
    auto callbacks = std::make_shared<detail::CallbackList>();
    for (const auto& entry : *state.callbacks) {
      if (entry.id != id) {
        callbacks->push_back(entry);
      }
    }
    state.anyCallback.store(!callbacks->empty(), std::memory_order_relaxed);
    state.callbacks = std::move(callbacks);
  }
};

/**
 * A fatal error, which ends the program when it is complete, i.e. at the end of the statement that
 * makes it
 *
 * The destructor does not return, and says so: code after a fatal error needs neither a return
 * statement nor a call to std::abort(). As each FatalLogger has to end the program, there is no
 * way to copy or move one.
 */
class FatalLogger : public LogMessage {
  public:
  explicit FatalLogger(bool broadcast, const LogLocation& location = LogLocation())
      : LogMessage(LogLevel::Error, broadcast, location) {}

  FatalLogger(const FatalLogger&) = delete;
  FatalLogger(FatalLogger&&) = delete;
  auto operator=(const FatalLogger&) -> FatalLogger& = delete;
  auto operator=(FatalLogger&&) -> FatalLogger& = delete;

  [[noreturn]] ~FatalLogger() {
    finish();
    detail::endProgram(finishedRecord());
  }
};

/**
 * Function to activate automatic spacing
 *
 * Example:
 * <code>logInfo() << nospace() << x << ":";</code>
 *
 * @relates utils::LogMessage
 */
inline auto space(LogMessage& message) -> LogMessage& { return message.space(); }

/**
 * Function to deactivate automatic spacing
 *
 * @see space()
 * @relates utils::LogMessage
 */
inline auto nospace(LogMessage& message) -> LogMessage& { return message.nospace(); }

/**
 * Dummy Logger class, does nothing
 */
class NoLogger {
  public:
  NoLogger() = default;
  ~NoLogger() = default;

  /**
   * Do nothing with the message
   */
  template <typename T>
  auto operator<<(const T& /*unused*/) -> NoLogger& {
    return *this;
  }

  /**
   * Operator to add functions like std::endl
   */
  auto operator<<(std::ostream& (* /*func*/)(std::ostream&)) -> NoLogger& { return *this; }

  /**
   * Operator for enabling/disabling automatic spacing
   * (the operator itself is ignored)
   */
  auto operator<<(LogMessage& (* /*func*/)(LogMessage&)) -> NoLogger& { return *this; }
};

} // namespace logger_v3
} // namespace utils

// Define global functions

/**
 * Which of the functions below are real loggers and which are dummies depends on LOG_LEVEL, which
 * may differ between translation units. They live in an inline namespace named after the level, so
 * that each level has symbols of its own, and the linker never merges the dummy of one translation
 * unit with the logger of another.
 */
inline namespace UTILS_LOGGER_LEVEL_NAMESPACE {

namespace utils_logger_detail {

/** The LOG_LEVEL from which on messages of a level are compiled in */
constexpr auto compiledFrom(utils::LogLevel level) -> int {
  switch (level) {
  case utils::LogLevel::Debug:
    return 3;
  case utils::LogLevel::Info:
    return 2;
  case utils::LogLevel::Warning:
    return 1;
  case utils::LogLevel::Error:
    return 0;
  }
  return 0;
}

/** A message of Level, or a dummy where LOG_LEVEL leaves out messages of Level */
template <utils::LogLevel Level>
auto logAt(bool broadcast, const utils::LogLocation& location) {
  if constexpr (Level == utils::LogLevel::Error) {
    return utils::FatalLogger(broadcast, location);
  } else if constexpr (LOG_LEVEL >= compiledFrom(Level)) {
    return utils::Logger(Level, broadcast, location);
  } else {
    static_cast<void>(broadcast);
    static_cast<void>(location);
    return utils::NoLogger();
  }
}

} // namespace utils_logger_detail

/**
 * Create a fatal error message, which ends the program
 *
 * @relates utils::FatalLogger
 */
inline auto logFatal(bool broadcast = true,
                     utils::LogLocation location = utils::LogLocation::current()) {
  return utils_logger_detail::logAt<utils::LogLevel::Error>(broadcast, location);
}

/**
 * Create error message and exit; the same as logFatal()
 *
 * @relates utils::FatalLogger
 */
inline auto logError(bool broadcast = true,
                     utils::LogLocation location = utils::LogLocation::current()) {
  return utils_logger_detail::logAt<utils::LogLevel::Error>(broadcast, location);
}

/**
 * Create a warning message if enabled
 *
 * @relates utils::Logger
 */
inline auto logWarning(bool broadcast = false,
                       utils::LogLocation location = utils::LogLocation::current()) {
  return utils_logger_detail::logAt<utils::LogLevel::Warning>(broadcast, location);
}

/**
 * Create a info message if enabled
 *
 * @relates utils::Logger
 */
inline auto logInfo(bool broadcast = false,
                    utils::LogLocation location = utils::LogLocation::current()) {
  return utils_logger_detail::logAt<utils::LogLevel::Info>(broadcast, location);
}

/**
 * Create a debug message if enabled
 *
 * @relates utils::Logger
 */
inline auto logDebug(bool broadcast = false,
                     utils::LogLocation location = utils::LogLocation::current()) {
  return utils_logger_detail::logAt<utils::LogLevel::Debug>(broadcast, location);
}

} // namespace UTILS_LOGGER_LEVEL_NAMESPACE

#endif // UTILS_LOGGER_H_
