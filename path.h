// SPDX-FileCopyrightText: 2014-2024 Technical University of Munich
//
// SPDX-License-Identifier: BSD-3-Clause
//
// SPDX-FileContributor: Sebastian Rettenberger

#ifndef UTILS_PATH_H_
#define UTILS_PATH_H_

#include <cstddef>
#include <string>
#include <sys/stat.h>
#include <utility>

#include "utils/stringutils.h"

namespace utils {

/**
 * Manipulates file/directory names and paths
 */
class Path {
  private:
  std::string m_path;

  public:
  Path() = default;

  Path(const char* path) : m_path(path) { init(); }

  Path(std::string path) : m_path(std::move(path)) { init(); }

  /**
   * @return The string representing the current path
   */
  operator std::string() const { return m_path; }

  /**
   * @return The basename of the path
   */
  [[nodiscard]] auto basename() const -> std::string {
    const size_t lastSlash = m_path.find_last_of(separators());
    if (lastSlash == std::string::npos) {
      return m_path;
    }

    return m_path.substr(lastSlash + 1);
  }

  /**
   * @return The directory name of the path
   */
  [[nodiscard]] auto dirname() const -> std::string {
    const size_t lastSlash = m_path.find_last_of(separators());
    if (lastSlash == std::string::npos) {
      return "";
    }

    return m_path.substr(0, lastSlash);
  }

  /**
   * @return The directory of the path
   */
  [[nodiscard]] auto dir() const -> Path { return Path(dirname()); }

  /**
   * @return True of the path is a file/directory
   */
  [[nodiscard]] auto exists() const -> bool {
    struct stat buffer{};
    return (stat(m_path.c_str(), &buffer) == 0);
  }

  /**
   * Joins two paths
   *
   * @param other The path that should be appended (has to be relative)
   * @return A path where other is appended to the current path
   */
  auto operator+(const Path& other) const -> Path {
    if (m_path.empty()) {
      return other;
    }
    if (other.m_path.empty()) {
      return *this;
    }

    return Path(m_path + SEPARATOR + other.m_path);
  }

  static auto separator() -> const char* {
    static const std::string Sep(1, SEPARATOR);
    return Sep.c_str();
  }

  /**
   * @return A string containing all possible separators
   */
  static auto separators() -> const char* {
#if __unix__
    return "/";
#else  // __unix__
    return "\\/";
#endif // __unix__
  }

  private:
  void init() {
    // Remove trailing separator
    if (StringUtils::endsWith(m_path, separator())) {
      StringUtils::replaceLast(m_path, separator(), "");
    }
  }

  public:
#ifdef __unix__
  static const char SEPARATOR = '/';
#else  // __unix__
  static const char SEPARATOR = '\\';
#endif // __unix__
};

} // namespace utils

#endif // UTILS_PATH_H_
