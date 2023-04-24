#pragma once

#ifdef _WIN32
#include <windows.h>
#endif

namespace xpano::cli::signal {

#ifdef _WIN32
void RegisterInterruptHandler(PHANDLER_ROUTINE handler);
#else
typedef void (*signal_handler)(int);

void RegisterInterruptHandler(signal_handler handler);
#endif

}  // namespace xpano::cli::signal
