// SPDX-FileCopyrightText: 2014-2024 Technical University of Munich
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef UTILS_MATHUTILS_H_
#define UTILS_MATHUTILS_H_

namespace utils {

/**
 * A collection of useful math functions
 */
class MathUtils {
  MathUtils() = delete;

public:
  /**
   * Finds the smallest value x >= a such that x % k == 0
   *
   * a and k should be of kind "int".
   */
  template <typename T> static T roundUp(T a, T k) {
    return ((a + k - 1) / k) * k;
  }

  /**
   * Computes the greatest common divisor of a and b
   *
   * @param a
   * @param b
   * @return
   */
  template <typename T> static T gcd(T a, T b) {
    T c = a % b;
    while (c != 0) {
      a = b;
      b = c;
      c = a % b;
    }
    return b;
  }
};

} // namespace utils

#endif // UTILS_MATHUTILS_H_
