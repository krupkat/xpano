#include "xpano/cli/windows_console.h"

#include <cstdio>
#include <windows.h>

#include <spdlog/spdlog.h>

namespace xpano::cli::windows {

// This is needed to redirect stdout to the console on Windows, because we are
// building with the WIN32 subsystem (app with no console window).

Attach::Attach() {
  if (AttachConsole(ATTACH_PARENT_PROCESS)) {
    attached_console_ = true;
    freopen_s(&attached_stdout_, "CONOUT$", "w", stdout);

    // This is needed to handle ctrl-c events by the system
    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), ENABLE_PROCESSED_INPUT);
  }
}

Attach::~Attach() {
  if (attached_stdout_) {
    fflush(attached_stdout_);
    fclose(attached_stdout_);
  }
  if (attached_console_) {
    FreeConsole();
  }
}

}  // namespace xpano::cli::windows
