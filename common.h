// SPDX-FileCopyrightText: 2016-2024 Technical University of Munich
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef UTILS_COMMON_H_
#define UTILS_COMMON_H_

#include <utility>

namespace utils {

// available since C++11(?)
template <typename T> [[deprecated]] void swap(T &a, T &b) {
  T tmp = a;
  a = b;
  b = tmp;
}

template <typename T> struct has_size {
  template <typename U>
  static constexpr decltype(std::declval<U>().size(), bool()) test(int) {
    return true;
  }
  template <typename U> static constexpr bool test(...) { return false; }
  static constexpr bool value = test<T>(int());
};

} // namespace utils

#endif // UTILS_COMMON_H_
