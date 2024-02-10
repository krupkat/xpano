// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xpano/cli/signal.h"

#ifdef _WIN32
#include <windows.h>

#include <spdlog/spdlog.h>
#else
#include <signal.h>  // NOLINT(modernize-deprecated-headers)
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
  struct sigaction action;
  action.sa_handler = handler;
  sigfillset(&action.sa_mask);
  action.sa_flags = SA_RESETHAND;
  sigaction(SIGINT, &action, nullptr);
}
#endif

}  // namespace xpano::cli::signal
