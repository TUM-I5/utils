// SPDX-FileCopyrightText: 2013-2024 Technical University of Munich
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef UTILS_LOGGER_H_
#define UTILS_LOGGER_H_

#include "utils/common.h"
#include "utils/stringutils.h"
#include "utils/timeutils.h"

#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <ctime>
#include <execinfo.h>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>

#ifndef LOG_LEVEL
#ifdef NDEBUG
#define LOG_LEVEL 2
#else // NDEBUG
#define LOG_LEVEL 3
#endif // NDEBUG
#endif // LOG_LEVEL

#ifndef LOG_PREFIX
#define LOG_PREFIX ""
#endif // DEBUG_PRFIX

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

/**
 * A collection of useful utility functions
 */
namespace utils {

/**
 * Handles debugging/logging output
 *
 * Most of the code is taken from QDebug form the Qt Framework
 */
class Logger {
  public:
  /** Message type */
  enum class DebugType {
    /** A debug messages */
    LogDebug,
    /** A info message (printed to stdout) */
    LogInfo,
    /** A warning message */
    LogWarning,
    /** A fatal error */
    LogError
  };

  private:
  static inline int displayRank{0};
  static inline int rank{-1};
  static inline bool logAll{false};

  /** Contains all information for a debug message */
  struct Stream {
    /** The debug type */
    DebugType type;
    /** MPI Rank, set to 0 to print message */
    int rank;
    /** References */
    int ref{1};
    /** Buffer for the output */
    std::stringstream buffer;
    /** Print additional space */
    bool space{true};

    bool broadcast;

    /**
     * Set defaults for a debug message
     */
    Stream(DebugType t, int r, bool broadcast)
        : type(t), rank(r), buffer(std::stringstream::out), broadcast(broadcast) {}
  }* stream;
  /**
   * Pointer to all information about the message
   */

  template <typename T, std::size_t Idx>
  static void printTuple(Logger& logger, const T& data) {
    if constexpr (Idx < std::tuple_size_v<T>) {
      if constexpr (Idx > 0) {
        logger << ", ";
      }
      logger << std::get<Idx>(data);
      printTuple<T, Idx + 1>(logger, data);
    }
  }

  public:
  static void setDisplayRank(int rank) { Logger::displayRank = rank; }
  static void setRank(int rank) { Logger::rank = rank; }
  static void setLogAll(bool logAll) { Logger::logAll = logAll; }

  /**
   * Start a new Debug message
   *
   * @param t Type of the message
   * @param rank Rank of the current process, only messages form rank
   *  0 will be printed
   */
  Logger(DebugType t, bool broadcast) : stream(new Stream(t, Logger::rank, broadcast)) {
    auto timepoint = std::chrono::system_clock::now();
    auto milliTotal =
        std::chrono::duration_cast<std::chrono::milliseconds>(timepoint.time_since_epoch()).count();
    auto milli = milliTotal % 1000;
    const time_t time = std::chrono::system_clock::to_time_t(timepoint);

    stream->buffer << utils::TimeUtils::timeAsString("%F %T", time) << "."
                   << StringUtils::padLeft(std::to_string(milli), 3, '0');

    switch (t) {
    case DebugType::LogDebug:
      stream->buffer << " debug ";
      break;
    case DebugType::LogInfo:
      stream->buffer << " info ";
      break;
    case DebugType::LogWarning:
      stream->buffer << " warn ";
      break;
    case DebugType::LogError:
      stream->buffer << " error ";
      break;
    default:
      stream->buffer << " unknown ";
      break;
    }

    if (stream->rank >= 0) {
      stream->buffer << stream->rank << " : ";
    } else {
      stream->buffer << "- : ";
    }
  }
  /**
   * Copy constructor
   */
  Logger(const Logger& o) : stream(o.stream) { stream->ref++; };
  ~Logger() {
    if (--stream->ref == 0) {
      if (stream->rank == Logger::displayRank || stream->rank == -1 || Logger::logAll ||
          stream->broadcast) {
        if (stream->type == DebugType::LogInfo || stream->type == DebugType::LogDebug) {
          std::cout << stream->buffer.str() << '\n';
        } else {
          std::cerr << stream->buffer.str() << '\n';
        }
      }

      if (stream->type == DebugType::LogError) {
        delete stream;
        stream = nullptr; // Avoid double free if LOG_ABORT does
                          // does not exit the program

        // Backtrace
        if (BACKTRACE_SIZE > 0) {
          void* buffer[BACKTRACE_SIZE];
          const int nptrs = backtrace(buffer, BACKTRACE_SIZE);
          char** strings = backtrace_symbols(buffer, nptrs);

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

        std::raise(SIGTRAP);

        LOG_ABORT;
      }

      delete stream;
    }
  }

  /**
   * Copy operator
   */
  auto operator=(const Logger& other) -> Logger& {
    if (this != &other) {
      Logger copy(other);
      std::swap(stream, copy.stream);
    }
    return *this;
  }

  /********* Space handling *********/

  /**
   * Add a space to output message and activate spaces
   */
  auto space() -> Logger& {
    stream->space = true;
    stream->buffer << ' ';
    return *this;
  }
  /**
   * Deactivate spaces
   */
  auto nospace() -> Logger& {
    stream->space = false;
    return *this;
  }
  /**
   * Add space of activated
   */
  auto maybeSpace() -> Logger& {
    if (stream->space) {
      stream->buffer << ' ';
    }
    return *this;
  }

  /**
   * Default function to add messages
   */
  template <typename T>
  auto operator<<(const T& data) -> Logger& {
    if constexpr (std::is_invocable_r_v<Logger&, T, Logger&>) {
      return std::invoke(data, *this);
    } else if constexpr (std::is_same_v<T, std::string>) {
      stream->buffer << '"' << data << '"';
      return maybeSpace();
    } else if constexpr (CanOutput<T>::Value) {
      stream->buffer << data;
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
  auto operator<<(std::ostream& (*func)(std::ostream&)) -> Logger& {
    stream->buffer << func;
    return *this; // No space in this case
  }
};

/**
 * Function to activate automatic spacing
 *
 * Example:
 * <code>logInfo() << nospace() << x << ":";</code>
 *
 * @relates utils::Logger
 */
inline auto space(Logger& logger) -> Logger& { return logger.space(); }

/**
 * Function to deactivate automatic spacing
 *
 * @see space()
 * @relates utils::Logger
 */
inline auto nospace(Logger& logger) -> Logger& { return logger.nospace(); }

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
  auto operator<<(std::ostream& (*func)(std::ostream&)) -> NoLogger& { return *this; }

  /**
   * Operator for enabling/disabling automatic spacing
   * (the operator itself is ignored)
   */
  auto operator<<(Logger& (*func)(Logger&)) -> NoLogger& { return *this; }
};

} // namespace utils

// Define global functions

/**
 * Create error message and exit
 *
 * @relates utils::Logger
 */
inline auto logError(bool broadcast = true) -> utils::Logger {
  return {utils::Logger::DebugType::LogError, broadcast};
}

#if LOG_LEVEL >= 1
/**
 * Create a warning message if enabled
 *
 * @relates utils::Logger
 */
inline auto logWarning(bool broadcast = false) -> utils::Logger {
  return {utils::Logger::DebugType::LogWarning, broadcast};
}
#else  // LOG_LEVEL >= 1
/**
 * Create a dummy warning message if disabled
 *
 * @relates utils::NoLogger
 */
inline utils::NoLogger logWarning(bool broadcast = false) { return utils::NoLogger(); }
#endif // LOG_LEVEL >= 1

#if LOG_LEVEL >= 2
/**
 * Create a info message if enabled
 *
 * @relates utils::Logger
 */
inline auto logInfo(bool broadcast = false) -> utils::Logger {
  return {utils::Logger::DebugType::LogInfo, broadcast};
}
#else  // LOG_LEVEL >= 2
/**
 * Create a dummy info message if disabled
 *
 * @relates utils::NoLogger
 */
inline utils::NoLogger logInfo(bool broadcast = false) { return utils::NoLogger(); }
#endif // LOG_LEVEL >= 2

#if LOG_LEVEL >= 3
/**
 * Create a debug message if enabled
 *
 * @relates utils::Logger
 */
inline auto logDebug(bool broadcast = false) -> utils::Logger {
  return {utils::Logger::DebugType::LogDebug, broadcast};
}
#else  // LOG_LEVEL >= 3
/**
 * Create a dummy debug message if disabled
 *
 * @relates utils::NoLogger
 */
inline utils::NoLogger logDebug(bool broadcast = false) { return utils::NoLogger(); }
#endif // LOG_LEVEL >= 3

// Use for variables unused when compiling with NDEBUG
#ifdef NDEBUG
#define NDBG_UNUSED(x) ((void)x)
#else // NDEBUG
#define NDBG_UNUSED(x)
#endif // NDEBUG

#endif // UTILS_LOGGER_H_
