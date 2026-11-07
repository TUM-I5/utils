// SPDX-FileCopyrightText: 2026 Technical University of Munich
//
// SPDX-License-Identifier: BSD-3-Clause
//
// SPDX-FileContributor: David Schneller

// Compiled with LOG_LEVEL=1, unlike the other translation unit of the logger test, so that the
// logger functions of two levels end up in one program.

#include "utils/logger.h"

#if LOG_LEVEL != 1
#error "logger_level.cpp is meant to be compiled with LOG_LEVEL=1"
#endif

namespace logger_test {

void logAtLevelOne() {
  logDebug() << "debug at level 1";
  logInfo() << "info at level 1";
  logWarning() << "warning at level 1";
}

} // namespace logger_test
