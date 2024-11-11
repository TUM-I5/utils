// SPDX-FileCopyrightText: 2016-2024 Technical University of Munich
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef UTILS_COMMON_H_
#define UTILS_COMMON_H_

#include <ostream>
#include <utility>

namespace utils {

template <typename T>
struct HasSize {
  template <typename U>
  static constexpr auto test(int /*unused*/) -> decltype(std::declval<U>().size(), bool()) {
    return true;
  }
  template <typename U>
  static constexpr auto test(...) -> bool {
    return false;
  }
  static constexpr bool Value = test<T>(int());
};

template <typename T>
struct IsIterable {
  template <typename U>
  static constexpr auto beginTest(int /*unused*/)
      -> decltype(std::begin(std::declval<U>()), bool()) {
    return true;
  }
  template <typename U>
  static constexpr auto beginTest(...) -> bool {
    return false;
  }
  template <typename U>
  static constexpr auto endTest(int /*unused*/) -> decltype(std::end(std::declval<U>()), bool()) {
    return true;
  }
  template <typename U>
  static constexpr auto endTest(...) -> bool {
    return false;
  }
  static constexpr bool Value = beginTest<T>(int()) && endTest<T>(int());
};

template <typename T>
struct CanOutput {
  template <typename U>
  static constexpr auto test(int /*unused*/)
      -> decltype(operator<<(std::declval<std::ostream>(), std::declval<U>()), bool()) {
    return true;
  }
  template <typename U>
  static constexpr auto test(...) -> bool {
    return false;
  }
  static constexpr bool Value = test<T>(int());
};

template <typename T>
struct IsGettable {
  template <typename U>
  static constexpr auto test(int /*unused*/) -> decltype(std::tuple_size<U>::value, bool()) {
    return true;
  }
  template <typename U>
  static constexpr auto test(...) -> bool {
    return false;
  }
  static constexpr bool Value = test<T>(int());
};

} // namespace utils

#endif // UTILS_COMMON_H_
