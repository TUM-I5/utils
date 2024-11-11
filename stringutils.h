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
  public:
  /**
   * Replaces first occurrence of from in str with to
   *
   * Taken from
   * http://stackoverflow.com/questions/3418231/c-replace-part-of-a-string-with-another-string
   */
  static auto replace(std::string& str, const std::string& from, const std::string& to) -> bool {
    const size_t startPos = str.find(from);
    if (startPos == std::string::npos) {
      return false;
    }
    str.replace(startPos, from.length(), to);
    return true;
  }

  /**
   * Replaces last occurrence of from in str with to
   */
  static auto replaceLast(std::string& str, const std::string& from, const std::string& to)
      -> bool {
    const size_t startPos = str.rfind(from);
    if (startPos == std::string::npos) {
      return false;
    }
    str.replace(startPos, from.length(), to);
    return true;
  }

  static auto startsWith(const std::string& str, const std::string& prefix) -> bool {
    return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
  }

  /**
   * Checks a string for specific suffix
   *
   * Taken from:
   * http://stackoverflow.com/questions/20446201/how-to-check-if-string-ends-with-txt
   */
  static auto endsWith(const std::string& str, const std::string& suffix) -> bool {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
  }

  static auto padLeft(const std::string& str, std::size_t size, char padchar) -> std::string {
    if (str.size() >= size) {
      return str;
    } else {
      std::stringstream stream;
      const std::size_t padlen = size - str.size();
      for (std::size_t i = 0; i < padlen; ++i) {
        stream << padchar;
      }
      stream << str;
      return stream.str();
    }
  }

  static auto padRight(const std::string& str, std::size_t size, char padchar) -> std::string {
    if (str.size() >= size) {
      return str;
    } else {
      std::stringstream stream;
      stream << str;
      const std::size_t padlen = size - str.size();
      for (std::size_t i = 0; i < padlen; ++i) {
        stream << padchar;
      }
      return stream.str();
    }
  }

  /**
   * Converts arbitrary datatypes (all datatypes which support the << stream
   * operator) into std::string
   */
  template <typename T>
  static auto toString(T&& value) -> std::string {
    std::ostringstream ss;
    ss << std::forward<T>(value);
    return ss.str();
  }

  /**
   * Converts strings to arbitrary datatypes (using the << stream operator)
   *
   * @param str The string that should be converted
   */
  template <typename T>
  static auto parse(const std::string& str) -> T {
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
  template <typename T>
  static auto parse(const std::string& str, bool adavanced) -> T {
    // By default the advanced mode is disabled for all datatypes
    return parse<T>(str);
  }

  template <typename T>
  static auto parseArray(const std::string& str) -> std::vector<T> {
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
  static void toUpper(char* s) {
    for (size_t i = 0; s[i] != '\0'; ++i) {
      s[i] = std::toupper(s[i]);
    }
  }

  /**
   * Converts std::string to upper case
   */
  static void toUpper(std::string& str) {
    for (auto& chr : str) {
      chr = std::toupper(chr);
    }
  }

  /**
   * Converts null terminated string to lower case
   */
  static void toLower(char* s) {
    for (size_t i = 0; s[i] != '\0'; ++i) {
      s[i] = std::tolower(s[i]);
    }
  }

  /**
   * Converts std::string to lower case
   */
  static void toLower(std::string& str) {
    for (auto& chr : str) {
      chr = std::tolower(chr);
    }
  }

  /**
   * Trims from start
   *
   * Taken from
   * http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
   */
  static auto ltrim(std::string& s) -> std::string& {
    const auto notSpace = [](unsigned char ch) { return std::isspace(ch) == 0; };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
    return s;
  }

  /**
   * Trims from end
   *
   * Taken from
   * http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
   */
  static auto rtrim(std::string& s) -> std::string& {
    const auto notSpace = [](unsigned char ch) { return std::isspace(ch) == 0; };
    s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
    return s;
  }

  /**
   * Trims from both ends
   *
   * Taken from
   * http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
   */
  static auto trim(std::string& s) -> std::string& { return ltrim(rtrim(s)); }

  /**
   * Join vector
   *
   * Taken from
   * http://dracoater.blogspot.com/2010/03/joining-vector-and-splitting-string-in.html
   */
  template <typename T>
  static auto join(const std::vector<T>& v, const std::string& token) -> std::string {
    std::ostringstream result;
    for (auto i = v.begin(); i != v.end(); ++i) {
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
  static auto split(const std::string& str, char delim) -> std::vector<std::string> {
    std::vector<std::string> elems;
    std::stringstream ss(str);

    std::string item;
    while (std::getline(ss, item, delim)) {
      elems.push_back(item);
    }

    return elems;
  }
};

template <>
inline auto StringUtils::parse(const std::string& str) -> std::string {
  return str;
}

template <>
inline auto StringUtils::parse(const std::string& str) -> const char* {
  return str.c_str();
}

template <>
inline auto StringUtils::parse(const std::string& str) -> bool {
  std::string s = str;
  toLower(s);
  trim(s);

  if (s == "on" || s == "yes" || s == "true") {
    return true;
  }
  if (s == "off" || s == "no" || s == "false") {
    return false;
  }

  return parse<bool>(str);
}

} // namespace utils

#endif // UTILS_STRINGUTILS_H_
