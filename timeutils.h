// SPDX-FileCopyrightText: 2013-2024 Technical University of Munich
//
// SPDX-License-Identifier: BSD-3-Clause
//
// SPDX-FileContributor: Sebastian Rettenberger

#ifndef UTILS_TIMEUTILS_H_
#define UTILS_TIMEUTILS_H_

#include <ctime>
#include <string>

/**
 * A collection of useful utility functions
 */
namespace utils {

/**
 * A collection of usefull time functions
 */
class TimeUtils {
  public:
  /**
   * Formats a string using strftime
   *
   * Taken from
   * http://stackoverflow.com/questions/7935483/c-function-to-format-time-t-as-stdstring-buffer-length
   *
   * @return A copy of formatString, with all %k replaced with the time
   * information
   */
  static auto timeAsString(const std::string& formatString, time_t time) -> std::string {
    const struct tm* timeinfo = localtime(&time);

    // strftime returns 0 both if the buffer is too small and if the result is empty (or the
    // format is not understood by the C runtime); thus, only grow the buffer up to a limit
    const size_t maxSize = formatString.size() * 64 + 256;

    std::string buffer;
    buffer.resize(formatString.size() * 2 + 1);
    size_t len = strftime(buffer.data(), buffer.size(), formatString.c_str(), timeinfo);
    while (len == 0 && buffer.size() < maxSize) {
      buffer.resize(buffer.size() * 2);
      len = strftime(buffer.data(), buffer.size(), formatString.c_str(), timeinfo);
    }
    buffer.resize(len);
    return buffer;
  }

  /**
   * @copydoc timeAsString(const std::string&, time_t)
   *
   * Returns the formated time for the current time
   */
  static auto timeAsString(const std::string& formatString) -> std::string {
    return timeAsString(formatString, time(nullptr));
  }
};

} // namespace utils

#endif // UTILS_TIMEUTILS_H_
