// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#ifdef _WIN32
#include <windows.h>
#endif

namespace xpano::cli::signal {

#ifdef _WIN32
void RegisterInterruptHandler(PHANDLER_ROUTINE handler);
#else
using SignalHandler = void (*)(int);

void RegisterInterruptHandler(SignalHandler handler);
#endif

}  // namespace xpano::cli::signal
