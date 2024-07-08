// SPDX-FileCopyrightText: 2013-2024 Technical University of Munich
//
// SPDX-License-Identifier: BSD-3-Clause

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
  TimeUtils() = delete;

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
  static std::string timeAsString(const std::string &formatString,
                                  time_t time) {
    const struct tm *timeinfo = localtime(&time);

    std::string buffer;
    buffer.resize(formatString.size() * 2);
    size_t len =
        strftime(&buffer[0], buffer.size(), formatString.c_str(), timeinfo);
    while (len == 0) {
      buffer.resize(buffer.size() * 2);
      len = strftime(&buffer[0], buffer.size(), formatString.c_str(), timeinfo);
    }
    buffer.resize(len);
    return buffer;
  }

  /**
   * @copydoc timeAsString(const std::string&, time_t)
   *
   * Returns the formated time for the current time
   */
  static std::string timeAsString(const std::string &formatString) {
    return timeAsString(formatString, time(0L));
  }
};

} // namespace utils

#endif // UTILS_TIMEUTILS_H_
