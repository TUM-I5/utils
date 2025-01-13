// SPDX-FileCopyrightText: 2014-2024 Technical University of Munich
//
// SPDX-License-Identifier: BSD-3-Clause
//
// SPDX-FileContributor: Sebastian Rettenberger
// SPDX-FileContributor: David Schneller

#ifndef UTILS_ENV_H_
#define UTILS_ENV_H_

#include <cstdlib>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>

#include "utils/stringutils.h"

namespace utils {

/**
 * Function(s) to handle environment variables
 */
class Env {
  private:
  static inline std::unordered_map<std::string, std::optional<std::string>> cache;

  public:
  template <typename T>
  static auto getOptional(const std::string& name) -> std::optional<T> {
    if (cache.find(name) == cache.end()) {
      char* value = std::getenv(name.c_str());
      if (value == nullptr) {
        cache[name] = std::optional<std::string>();
      } else {
        cache[name] = std::make_optional<std::string>(value);
      }
    }

    const auto value = cache.at(name);
    if (cache.at(name).has_value()) {
      return std::make_optional<T>(StringUtils::parse<T>(value.value()));
    } else {
      return std::optional<T>();
    }
  }

  template <typename T>
  static auto get(const std::string& name, T&& defaultVal)
      -> std::enable_if_t<!std::is_array_v<T>, std::decay_t<T>> {
    // mirror requirements for an optional
    const auto value = getOptional<std::decay_t<T>>(name);

    if (value.has_value()) {
      return value.value();
    } else {
      return std::forward<T>(defaultVal);
    }
  }

  template <typename T>
  static auto get(const std::string& name, const T* defaultVal)
      -> std::enable_if_t<std::is_convertible_v<T*, std::string>, std::string> {
    const auto value = getOptional<std::string>(name);

    if (value.has_value()) {
      return value.value();
    } else {
      return std::string(defaultVal);
    }
  }
};

} // namespace utils

#endif // UTILS_ENV_H_
