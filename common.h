// SPDX-FileCopyrightText: 2016-2024 Technical University of Munich
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef UTILS_COMMON_H_
#define UTILS_COMMON_H_

#include <ostream>
#include <utility>

namespace utils {

template <typename T> struct has_size {
  template <typename U>
  static constexpr decltype(std::declval<U>().size(), bool()) test(int) {
    return true;
  }
  template <typename U> static constexpr bool test(...) { return false; }
  static constexpr bool value = test<T>(int());
};

template <typename T> struct IsIterable {
  template <typename U>
  static constexpr decltype(std::begin(std::declval<U>()), bool())
  beginTest(int) {
    return true;
  }
  template <typename U> static constexpr bool beginTest(...) { return false; }
  template <typename U>
  static constexpr decltype(std::end(std::declval<U>()), bool()) endTest(int) {
    return true;
  }
  template <typename U> static constexpr bool endTest(...) { return false; }
  static constexpr bool Value = beginTest<T>(int()) && endTest<T>(int());
};

template <typename T> struct CanOutput {
  template <typename U>
  static constexpr decltype(operator<<(std::declval<std::ostream>(),
                                       std::declval<U>()),
                            bool())
  test(int) {
    return true;
  }
  template <typename U> static constexpr bool test(...) { return false; }
  static constexpr bool Value = test<T>(int());
};

template <typename T> struct IsGettable {
  template <typename U>
  static constexpr decltype(std::tuple_size<U>::value, bool()) test(int) {
    return true;
  }
  template <typename U> static constexpr bool test(...) { return false; }
  static constexpr bool Value = test<T>(int());
};

} // namespace utils

#endif // UTILS_COMMON_H_
