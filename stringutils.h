// SPDX-FileCopyrightText: 2013-2024 Technical University of Munich
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef UTILS_STRINGUTILS_H_
#define UTILS_STRINGUTILS_H_

#include <algorithm>
#include <cctype>
#include <cstring>
#include <functional>
#include <locale>
#include <sstream>
#include <string>
#include <vector>

/**
 * A collection of useful utility functions
 */
namespace utils {

/**
 * A collection of useful string functions based on std::string
 */
class StringUtils {
  StringUtils() = delete;

public:
  /**
   * Replaces first occurrence of from in str with to
   *
   * Taken from
   * http://stackoverflow.com/questions/3418231/c-replace-part-of-a-string-with-another-string
   */
  static bool replace(std::string &str, const std::string &from,
                      const std::string &to) {
    size_t start_pos = str.find(from);
    if (start_pos == std::string::npos) {
      return false;
    }
    str.replace(start_pos, from.length(), to);
    return true;
  }

  /**
   * Replaces last occurrence of from in str with to
   */
  static bool replaceLast(std::string &str, const std::string &from,
                          const std::string &to) {
    size_t start_pos = str.rfind(from);
    if (start_pos == std::string::npos) {
      return false;
    }
    str.replace(start_pos, from.length(), to);
    return true;
  }

  static bool startsWith(const std::string &str, const std::string &prefix) {
    return str.size() >= prefix.size() &&
           str.compare(0, prefix.size(), prefix) == 0;
  }

  /**
   * Checks a string for specific suffix
   *
   * Taken from:
   * http://stackoverflow.com/questions/20446201/how-to-check-if-string-ends-with-txt
   */
  static bool endsWith(const std::string &str, const std::string &suffix) {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
  }

  /**
   * Converts arbitrary datatypes (all datatypes which support the << stream
   * operator) into std::string
   */
  template <typename T> static std::string toString(T &&value) {
    std::ostringstream ss;
    ss << std::forward<T>(value);
    return ss.str();
  }

  /**
   * Converts strings to arbitrary datatypes (using the << stream operator)
   *
   * @param str The string that should be converted
   */
  template <typename T> static T parse(const std::string &str) {
    T result;
    std::istringstream(str) >> result;

    return result;
  }

  /**
   * Converts strings to arbitrary datatypes
   *
   * @param str The string that should be converted
   * @param advanced True of advanced conversions should be enabled
   */
  template <typename T> static T parse(const std::string &str, bool adavanced) {
    // By default the advanced mode is disabled for all datatypes
    return parse<T>(str);
  }

  template <typename T>
  inline static std::vector<T> parseArray(const std::string &str) {
    std::vector<T> elems;
    std::istringstream f(str);
    std::string s;
    while (std::getline(f, s, ':')) {
      elems.push_back(parse<T>(s));
    }

    return elems;
  }

  /**
   * Converts null terminated string to upper case
   */
  static void toUpper(char *s) {
    for (size_t i = 0; s[i]; i++) {
      s[i] = toupper(s[i]);
    }
  }

  /**
   * Converts std::string to upper case
   */
  static void toUpper(std::string &str) {
    for (size_t i = 0; str[i]; i++) {
      str[i] = toupper(str[i]);
    }
  }

  /**
   * Converts null terminated string to lower case
   */
  static void toLower(char *s) {
    for (size_t i = 0; s[i]; i++) {
      s[i] = tolower(s[i]);
    }
  }

  /**
   * Converts std::string to lower case
   */
  static void toLower(std::string &str) {
    for (size_t i = 0; str[i]; i++) {
      str[i] = tolower(str[i]);
    }
  }

  /**
   * Trims from start
   *
   * Taken from
   * http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
   */
  static std::string &ltrim(std::string &s) {
    auto notSpace = [](unsigned char ch) { return !std::isspace(ch); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
    return s;
  }

  /**
   * Trims from end
   *
   * Taken from
   * http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
   */
  static std::string &rtrim(std::string &s) {
    auto notSpace = [](unsigned char ch) { return !std::isspace(ch); };
    s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
    return s;
  }

  /**
   * Trims from both ends
   *
   * Taken from
   * http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
   */
  static std::string &trim(std::string &s) { return ltrim(rtrim(s)); }

  /**
   * Join vector
   *
   * Taken from
   * http://dracoater.blogspot.com/2010/03/joining-vector-and-splitting-string-in.html
   */
  template <typename T>
  static std::string join(const std::vector<T> &v, const std::string &token) {
    std::ostringstream result;
    for (auto i = v.begin(); i != v.end(); i++) {
      if (i != v.begin()) {
        result << token;
      }
      result << *i;
    }

    return result.str();
  }

  /**
   * Split a string
   *
   * Taken from
   * http://stackoverflow.com/questions/236129/how-to-split-a-string-in-c
   */
  static std::vector<std::string> split(const std::string &str, char delim) {
    std::vector<std::string> elems;
    std::stringstream ss(str);

    std::string item;
    while (std::getline(ss, item, delim)) {
      elems.push_back(item);
    }

    return elems;
  }
};

template <> inline std::string StringUtils::parse(const std::string &str) {
  return str;
}

template <> inline const char *StringUtils::parse(const std::string &str) {
  return str.c_str();
}

template <>
inline bool StringUtils::parse(const std::string &str, bool advanced) {
  std::string s = str;
  toLower(s);
  trim(s);

  if (s == "on" || s == "yes" || s == "true") {
    return true;
  }

  return parse<bool>(str);
}

} // namespace utils

#endif // UTILS_STRINGUTILS_H_
