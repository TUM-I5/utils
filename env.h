// SPDX-FileCopyrightText: 2014-2024 Technical University of Munich
//
// SPDX-License-Identifier: BSD-3-Clause
//
// SPDX-FileContributor: Sebastian Rettenberger
// SPDX-FileContributor: David Schneller

#ifndef UTILS_ENV_H_
#define UTILS_ENV_H_

#include <cstdlib>
#include <initializer_list>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "utils/logger.h"
#include "utils/stringutils.h"

namespace utils {

/**
 * Function(s) to handle environment variables
 *
 * An instance looks up a variable under one or more prefixes. The first prefix
 * is the canonical one; any variable found under a later prefix is reported as
 * deprecated. This allows an application to namespace the variables of a
 * library it embeds (e.g. SEISSOL_ASYNC_MODE) while keeping the plain names
 * (ASYNC_MODE) working.
 *
 * An instance looks up every variable once, and reports a deprecated one only
 * then.
 */
class Env {
  private:
  std::vector<std::string> prefixes;
  std::unordered_map<std::string, std::optional<std::string>> cache;

  auto lookup(const std::string& name) -> const std::optional<std::string>& {
    const auto cached = cache.find(name);
    if (cached != cache.end()) {
      return cached->second;
    }

    std::optional<std::string> found;
    for (std::size_t i = 0; i < prefixes.size(); ++i) {
      const char* value = std::getenv((prefixes[i] + name).c_str());
      if (value != nullptr) {
        if (i > 0) {
          logWarning() << "The environment variable" << (prefixes[i] + name) << "is deprecated. Use"
                       << (prefixes[0] + name) << "instead.";
        }
        found = std::make_optional<std::string>(value);
        break;
      }
    }

    return cache.emplace(name, std::move(found)).first->second;
  }

  public:
  Env(const std::string& prefix) : prefixes{prefix} {}

  Env(std::initializer_list<std::string> prefixes) : prefixes(prefixes) {}

  explicit Env(std::vector<std::string> prefixes) : prefixes(std::move(prefixes)) {}

  /**
   * @return The canonical prefix, i.e. the one that should be used in messages
   *  directed at the user.
   */
  [[nodiscard]] auto prefix() const -> const std::string& { return prefixes.front(); }

  template <typename T>
  auto getOptional(const std::string& name) -> std::optional<T> {
    const auto& value = lookup(name);

    if (value.has_value()) {
      return std::make_optional<T>(StringUtils::parse<T>(value.value()));
    } else {
      return std::optional<T>();
    }
  }

  template <typename T>
  auto get(const std::string& name, T&& defaultVal)
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
  auto get(const std::string& name, const T* defaultVal)
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
