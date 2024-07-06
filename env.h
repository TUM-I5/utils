/**
 * @file
 *  This file is part of UTILS
 *
 * @author Sebastian Rettenberger <sebastian.rettenberger@tum.de>
 *
 * @copyright Copyright (c) 2014-2015, Technische Universitaet Muenchen.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright notice
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 *  3. Neither the name of the copyright holder nor the names of its
 *     contributors may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UTILS_ENV_H
#define UTILS_ENV_H

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
  Env() = delete;
  static inline std::unordered_map<std::string, std::optional<std::string>>
      cache;

public:
  template <typename T>
  static std::optional<T> getOptional(const std::string &name) {
    if (cache.find(name) == cache.end()) {
      char *value = std::getenv(name.c_str());
      if (value == nullptr) {
        cache[name] = std::optional<std::string>();
      } else {
        cache[name] = std::make_optional<std::string>(value);
      }
    }

	const auto value = cache.at(name);
	if (cache.at(name).has_value()) {
		return std::make_optional<T>(StringUtils::parse<T>(value.value()));
	}
	else {
		return std::optional<T>();
	}
  }

  template <typename T> static std::enable_if_t<!std::is_array_v<T>,T> get(const std::string &name, T&& defaultVal) {
	// mirror requirements for an optional
    const auto value = getOptional<T>(name);

    if (value.has_value()) {
      return value.value();
    } else {
      return std::forward<T>(defaultVal);
    }
  }

  template <typename T> static std::enable_if_t<std::is_convertible_v<T*, std::string>,std::string> get(const std::string &name, const T* defaultVal) {
    const auto value = getOptional<std::string>(name);

    if (value.has_value()) {
      return value.value();
    } else {
      return std::string(defaultVal);
    }
  }
};

} // namespace utils

#endif // UTILS_ENV_H
