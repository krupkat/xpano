// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xpano/cli/windows_console.h"

#include <windows.h>  // IWYU pragma: keep
//
#include <consoleapi.h>
#include <processenv.h>
#include <stdio.h>  // NOLINT(modernize-deprecated-headers)
#include <WinBase.h>

namespace xpano::cli::windows {

// This is needed to redirect stdout to the console on Windows, because we are
// building with the WIN32 subsystem (app with no console window).

Attach::Attach() {
  if (AttachConsole(ATTACH_PARENT_PROCESS) != 0) {
    attached_console_ = true;
    freopen_s(&attached_stdout_, "CONOUT$", "w", stdout);

    // This is needed to handle ctrl-c events by the system
    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), ENABLE_PROCESSED_INPUT);
  }
}

Attach::~Attach() {
  if (attached_stdout_ != nullptr) {
    fflush(attached_stdout_);
    fclose(attached_stdout_);
  }
  if (attached_console_) {
    FreeConsole();
  }
}

}  // namespace xpano::cli::windows
