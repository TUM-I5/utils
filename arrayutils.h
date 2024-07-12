// SPDX-FileCopyrightText: 2015-2024 Technical University of Munich
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef UTILS_ARRAYUTILS_H_
#define UTILS_ARRAYUTILS_H_

#include "common.h"

#include <cstddef>
#include <type_traits>

namespace utils {

/**
 * Collection of useful functions for arrays
 */
class ArrayUtils {
  ArrayUtils() = delete;

public:
  // available since C++17

  /**
   * Size of a static array
   *
   * @param a The array
   * @return The size (number of elements) of the array
   */
  template <typename T, std::size_t N>
  [[deprecated]] static std::size_t size(const T (&a)[N]) {
    return N;
  }

  /**
   * Size of a zero length static array
   *
   * @param a The array
   * @return 0
   */
  template <typename T> [[deprecated]] static std::size_t size(const T *) {
    return 0;
  }

  /**
   * Size of a container
   *
   * @param a A container of any type
   *
   * @note This function has the same signature as the function for
   *  static arrays and can be used with automatic template argument deduction.
   */
  template <typename T>
  [[deprecated]] static typename std::enable_if_t<has_size<T>::value, size_t>
  size(const T &a) {
    return a.size();
  }
};

} // namespace utils

#endif // UTILS_ARRAYUTILS_H_
