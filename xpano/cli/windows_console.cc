#include "xpano/cli/windows_console.h"

#include <cstdio>

#ifdef _WIN32
// clang-format off
#include <windows.h>
#include <consoleapi.h>
// clang-format on
#endif

namespace xpano::cli::windows {

// This is needed to redirect stdout to the console on Windows, because we are
// building with the WIN32 subsystem (app with no console window).

// NOLINTNEXTLINE(modernize-use-equals-default)
Attach::Attach() {
#ifdef _WIN32
  if (AttachConsole(ATTACH_PARENT_PROCESS)) {
    attached_console_ = true;
    freopen_s(&attached_stdout_, "CONOUT$", "w", stdout);
  }
#endif
}

// NOLINTNEXTLINE(modernize-use-equals-default)
Attach::~Attach() {
#ifdef _WIN32
  if (attached_stdout_) {
    fflush(attached_stdout_);
    fclose(attached_stdout_);
  }
  if (attached_console_) {
    FreeConsole();
  }
#endif
}

}  // namespace xpano::cli::windows
