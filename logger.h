// SPDX-FileCopyrightText: 2013-2024 Technical University of Munich
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef UTILS_LOGGER_H_
#define UTILS_LOGGER_H_

#include "utils/stringutils.h"
#include "utils/timeutils.h"

#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <ctime>
#include <execinfo.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

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

#ifndef LOG_ALL_RANKS
#define LOG_ALL_RANKS 0
#endif

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
  enum DebugType {
    /** A debug messages */
    LOG_DEBUG,
    /** A info message (printed to stdout) */
    LOG_INFO,
    /** A warning message */
    LOG_WARNING,
    /** A fatal error */
    LOG_ERROR
  };

private:
  /** Contains all information for a debug message */
  struct Stream {
    /** The debug type */
    DebugType type;
    /** MPI Rank, set to 0 to print message */
    int rank;
    /** References */
    int ref;
    /** Buffer for the output */
    std::stringstream buffer;
    /** Print additional space */
    bool space;

    /**
     * Set defaults for a debug message
     */
    Stream(DebugType t, int r)
        : type(t), rank(r), ref(1), buffer(std::stringstream::out),
          space(true) {}
  } *stream;
  /**
   * Pointer to all information about the message
   */
public:
  /**
   * Start a new Debug message
   *
   * @param t Type of the message
   * @param rank Rank of the current process, only messages form rank
   *  0 will be printed
   */
  Logger(DebugType t, int rank) : stream(new Stream(t, rank)) {
    auto timepoint = std::chrono::system_clock::now();
    auto milli = std::chrono::duration_cast<std::chrono::milliseconds>(
                     timepoint.time_since_epoch())
                     .count();
    time_t time = std::chrono::system_clock::to_time_t(timepoint);

    stream->buffer << utils::TimeUtils::timeAsString("%F %T", time) << "."
                   << StringUtils::padLeft(std::to_string(milli), 3, ' ');

    switch (t) {
    case LOG_DEBUG:
      stream->buffer << " debug ";
      break;
    case LOG_INFO:
      stream->buffer << " info ";
      break;
    case LOG_WARNING:
      stream->buffer << " warning ";
      break;
    case LOG_ERROR:
      stream->buffer << " error ";
      break;
    default:
      stream->buffer << " unknown ";
      break;
    }

    if (rank >= 0) {
      stream->buffer << rank << " ";
    } else {
      stream->buffer << "- ";
    }
  }
  /**
   * Copy constructor
   */
  Logger(const Logger &o) : stream(o.stream) { stream->ref++; };
  ~Logger() {
    if (!--stream->ref) {
      if (stream->rank == 0 || stream->rank == -1 || LOG_ALL_RANKS) {
        if (stream->type == LOG_INFO || stream->type == LOG_DEBUG) {
          std::cout << stream->buffer.str() << std::endl;
        } else {
          std::cerr << stream->buffer.str() << std::endl;
        }
      }

      if (stream->type == LOG_ERROR) {
        std::raise(SIGTRAP);

        delete stream;
        stream = 0L; // Avoid double free if LOG_ABORT does
                     // does not exit the program

        // Backtrace
        if (BACKTRACE_SIZE > 0) {
          void *buffer[BACKTRACE_SIZE];
          int nptrs = backtrace(buffer, BACKTRACE_SIZE);
          char **strings = backtrace_symbols(buffer, nptrs);

          // Buffer output to avoid interlacing with other processes
          std::stringstream outputBuffer;
          outputBuffer << "Backtrace:" << std::endl;
          for (int i = 0; i < nptrs; i++) {
            outputBuffer << strings[i] << std::endl;
          }
          free(strings);

          // Write backtrace to stderr
          std::cerr << outputBuffer.str() << std::flush;
        }

        LOG_ABORT;
      }

      delete stream;
    }
  }

  /**
   * Copy operator
   */
  Logger &operator=(const Logger &other) {
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
  Logger &space() {
    stream->space = true;
    stream->buffer << ' ';
    return *this;
  }
  /**
   * Deactivate spaces
   */
  Logger &nospace() {
    stream->space = false;
    return *this;
  }
  /**
   * Add space of activated
   */
  Logger &maybeSpace() {
    if (stream->space) {
      stream->buffer << ' ';
    }
    return *this;
  }

  /**
   * Default function to add messages
   */
  template <typename T> Logger &operator<<(T t) {
    stream->buffer << t;
    return maybeSpace();
  }

  /**
   * Add a string variable to the message
   */
  Logger &operator<<(const std::string &t) {
    stream->buffer << '"' << t << '"';
    return maybeSpace();
  }

  /**
   * Add a string variable to the message
   */
  Logger &operator<<(std::string &t) {
    stream->buffer << '"' << t << '"';
    return maybeSpace();
  }

  /**
   * Operator to add functions like std::endl
   */
  Logger &operator<<(std::ostream &(*func)(std::ostream &)) {
    stream->buffer << func;
    return *this; // No space in this case
  }

  /**
   * Operator for enabling/disabling automatic spacing
   */
  Logger &operator<<(Logger &(*func)(Logger &)) {
    func(*this);
    return *this;
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
inline Logger &space(Logger &logger) { return logger.space(); }

/**
 * Function to deactivate automatic spacing
 *
 * @see space()
 * @relates utils::Logger
 */
inline Logger &nospace(Logger &logger) { return logger.nospace(); }

/**
 * Dummy Logger class, does nothing
 */
class NoLogger {
public:
  NoLogger() {};
  ~NoLogger() {};

  /**
   * Do nothing with the message
   */
  template <typename T> NoLogger &operator<<(const T &) { return *this; }

  /**
   * Operator to add functions like std::endl
   */
  NoLogger &operator<<(std::ostream &(*func)(std::ostream &)) { return *this; }

  /**
   * Operator for enabling/disabling automatic spacing
   * (the operator itself is ignored)
   */
  NoLogger &operator<<(Logger &(*func)(Logger &)) { return *this; }
};

/**
 * Add a std::vector<T> to the message
 *
 * @relates utils::Logger
 */
template <typename T>
inline Logger &operator<<(Logger debug, const std::vector<T> &list) {
  debug.nospace() << '(';
  for (size_t i = 0; i < list.size(); i++) {
    if (i)
      debug << ", ";
    debug << list[i];
  }
  debug << ')';

  return debug.space();
}

} // namespace utils

// Define global functions

/**
 * Create error message and exit
 *
 * @relates utils::Logger
 */
inline utils::Logger logError(int rank = -1) {
  return utils::Logger(utils::Logger::LOG_ERROR, rank);
}

#if LOG_LEVEL >= 1
/**
 * Create a warning message if enabled
 *
 * @relates utils::Logger
 */
inline utils::Logger logWarning(int rank = -1) {
  return utils::Logger(utils::Logger::LOG_WARNING, rank);
}
#else  // LOG_LEVEL >= 1
/**
 * Create a dummy warning message if disabled
 *
 * @relates utils::NoLogger
 */
inline utils::NoLogger logWarning(int = -1) { return utils::NoLogger(); }
#endif // LOG_LEVEL >= 1

#if LOG_LEVEL >= 2
/**
 * Create a info message if enabled
 *
 * @relates utils::Logger
 */
inline utils::Logger logInfo(int rank = -1) {
  return utils::Logger(utils::Logger::LOG_INFO, rank);
}
#else  // LOG_LEVEL >= 2
/**
 * Create a dummy info message if disabled
 *
 * @relates utils::NoLogger
 */
inline utils::NoLogger logInfo(int = -1) { return utils::NoLogger(); }
#endif // LOG_LEVEL >= 2

#if LOG_LEVEL >= 3
/**
 * Create a debug message if enabled
 *
 * @relates utils::Logger
 */
inline utils::Logger logDebug(int rank = -1) {
  return utils::Logger(utils::Logger::LOG_DEBUG, rank);
}
#else  // LOG_LEVEL >= 3
/**
 * Create a dummy debug message if disabled
 *
 * @relates utils::NoLogger
 */
inline utils::NoLogger logDebug(int = -1) { return utils::NoLogger(); }
#endif // LOG_LEVEL >= 3

// Use for variables unused when compiling with NDEBUG
#ifdef NDEBUG
#define NDBG_UNUSED(x) ((void)x)
#else // NDEBUG
#define NDBG_UNUSED(x)
#endif // NDEBUG

#endif // UTILS_LOGGER_H_
