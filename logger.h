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
#include <cstdlib>
#include <ctime>
#include <functional>
#include <iostream>
#include <iterator>
#include <mutex>
#include <ostream>
#include <signal.h>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <tuple>
#include <type_traits>

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

#ifndef LOG_ABORT
#ifdef MPI_VERSION
#define LOG_ABORT MPI_Abort(MPI_COMM_WORLD, 134)
#else // MPI_VERSION
#define LOG_ABORT abort()
#endif // MPI_VERSION
#endif // LOG_ABORT

#ifndef BACKTRACE_SIZE
#define BACKTRACE_SIZE 50
#endif // BACKTRACE_SIZE

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
  /** A fatal error, which ends the program */
  Error
};

/** When a message was made: seconds and nanoseconds since the Unix epoch */
struct LogTime {
  std::int64_t seconds{0};
  std::int32_t nanoseconds{0};
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
};

namespace detail {

/** What all messages share: the settings of the logger, and the lock for handing messages on */
struct LoggerState {
  std::atomic<int> rank{-1};
  std::atomic<int> displayRank{0};
  std::atomic<bool> logAll{false};
  std::atomic<LogLevel> minimumLevel{LogLevel::Debug};
  std::mutex mutex;
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

/** Hands a finished message on to where messages go */
inline void dispatch(const LogRecord& record) noexcept {
  try {
    if (dispatching()) {
      // made while handing on another message: write it right away, without waiting for the lock
      if (shownOnConsole(record)) {
        writeToConsole(record);
      }
      return;
    }
    const std::lock_guard<std::mutex> lock(loggerState().mutex);
    const DispatchScope scope;
    if (shownOnConsole(record)) {
      writeToConsole(record);
    }
  } catch (...) {
    // a message that cannot be handed on is lost; the program goes on
  }
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
    }
    return *this;
  }

  /**
   * Default function to add messages
   */
  template <typename T>
  auto operator<<(const T& data) -> LogMessage& {
    if (!active) {
      return *this;
    }
    if constexpr (std::is_invocable_r_v<LogMessage&, T, LogMessage&>) {
      return std::invoke(data, *this);
    } else if constexpr (std::is_same_v<T, std::string>) {
      buffer << '"' << data << '"';
      return maybeSpace();
    } else if constexpr (CanOutput<T>::Value) {
      buffer << data;
      return maybeSpace();
    } else if constexpr (IsIterable<T>::Value) {
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

      return space();
    } else if constexpr (IsGettable<T>::Value) {
      nospace() << '{';
      printTuple<T, 0>(*this, data);
      *this << '}';

      return space();
    } else {
      // https://stackoverflow.com/questions/38304847/how-does-a-failed-static-assert-work-in-an-if-constexpr-false-block#comment119622305_64354296
      static_assert(sizeof(T) == 0, "Output for the given type not implemented.");
    }
  }

  /**
   * Operator to add functions like std::endl
   */
  auto operator<<(std::ostream& (*func)(std::ostream&)) -> LogMessage& {
    if (active) {
      buffer << func;
    }
    return *this; // No space in this case
  }

  protected:
  /**
   * Starts a message
   *
   * @param level How severe the message is
   * @param broadcast Whether every rank shows the message, and not only the display rank
   */
  LogMessage(LogLevel level, bool broadcast) {
    const auto& state = detail::loggerState();
    record.level = level;
    record.broadcast = broadcast;
    record.rank = state.rank.load(std::memory_order_relaxed);
    record.time = detail::now();
    active =
        level == LogLevel::Error || (level >= state.minimumLevel.load(std::memory_order_relaxed) &&
                                     detail::shownOnConsole(record));
  }

  ~LogMessage() = default;

  /** Hands the message on, if it goes anywhere */
  void finish() noexcept {
    if (active) {
      try {
        record.text = buffer.str();
      } catch (...) {
        return;
      }
      detail::dispatch(record);
    }
  }

  [[nodiscard]] auto level() const -> LogLevel { return record.level; }

  private:
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
};

/**
 * Handles debugging/logging output
 *
 * A Logger is a message that is handed on when it is destroyed, i.e. at the end of the statement
 * that makes it. Its static functions set how the logger works.
 */
class Logger : public LogMessage {
  public:
  Logger(LogLevel level, bool broadcast) : LogMessage(level, broadcast) {}

  Logger(const Logger&) = delete;
  Logger(Logger&&) = delete;
  auto operator=(const Logger&) -> Logger& = delete;
  auto operator=(Logger&&) -> Logger& = delete;

  ~Logger() {
    finish();

    if (level() == LogLevel::Error) {
#ifdef UTILS_LOGGER_HAS_BACKTRACE
      // Backtrace
      if (BACKTRACE_SIZE > 0) {
        void* frames[BACKTRACE_SIZE];
        const int nptrs = backtrace(frames, BACKTRACE_SIZE);
        char** strings = backtrace_symbols(frames, nptrs);

        // Buffer output to avoid interlacing with other processes
        std::stringstream outputBuffer;
        outputBuffer << "Backtrace:" << '\n';
        for (int i = 0; i < nptrs; i++) {
          outputBuffer << strings[i] << '\n';
        }
        free(strings);

        // Write backtrace to stderr
        std::cerr << outputBuffer.str() << std::flush;
      }
#endif // UTILS_LOGGER_HAS_BACKTRACE

      // Windows has no SIGTRAP; and raising an unknown signal there would end up in the
      // invalid parameter handler of the C runtime instead
#if defined(SIGTRAP) && !defined(_WIN32)
      std::raise(SIGTRAP);
#endif

      LOG_ABORT;
    }
  }

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
auto logAt(bool broadcast) {
  if constexpr (LOG_LEVEL >= compiledFrom(Level)) {
    return utils::Logger(Level, broadcast);
  } else {
    return utils::NoLogger();
  }
}

} // namespace utils_logger_detail

/**
 * Create error message and exit
 *
 * @relates utils::Logger
 */
inline auto logError(bool broadcast = true) {
  return utils_logger_detail::logAt<utils::LogLevel::Error>(broadcast);
}

/**
 * Create a warning message if enabled
 *
 * @relates utils::Logger
 */
inline auto logWarning(bool broadcast = false) {
  return utils_logger_detail::logAt<utils::LogLevel::Warning>(broadcast);
}

/**
 * Create a info message if enabled
 *
 * @relates utils::Logger
 */
inline auto logInfo(bool broadcast = false) {
  return utils_logger_detail::logAt<utils::LogLevel::Info>(broadcast);
}

/**
 * Create a debug message if enabled
 *
 * @relates utils::Logger
 */
inline auto logDebug(bool broadcast = false) {
  return utils_logger_detail::logAt<utils::LogLevel::Debug>(broadcast);
}

} // namespace UTILS_LOGGER_LEVEL_NAMESPACE

#endif // UTILS_LOGGER_H_
