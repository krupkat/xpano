#include "xpano/cli/signal.h"

#include <spdlog/spdlog.h>

#ifdef _WIN32
#include <windows.h>
#endif

namespace xpano::cli::signal {

#ifdef _WIN32
void RegisterInterruptHandler(PHANDLER_ROUTINE handler) {
  if (SetConsoleCtrlHandler(handler, TRUE) == 0) {
    spdlog::warn("Failed to register console ctrl handler");
  }
}
#else
void RegisterInterruptHandler(SignalHandler handler) {
  // unimplemented
}
#endif

}  // namespace xpano::cli::signal