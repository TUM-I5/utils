// SPDX-FileCopyrightText: 2026 Technical University of Munich
//
// SPDX-License-Identifier: BSD-3-Clause
//
// SPDX-FileContributor: David Schneller

// Compiled with -Werror=return-type where the compiler has that: the function below has no return
// statement after its fatal error, which compiles only if the compiler knows that a fatal error
// does not return.

#include "utils/logger.h"

namespace logger_test {

auto positiveOrFatal(int value) -> int {
  if (value > 0) {
    return value;
  }
  logFatal() << "not positive:" << value;
}

} // namespace logger_test
