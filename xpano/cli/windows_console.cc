#include "xpano/cli/windows_console.h"

#include <stdio.h>

#ifdef _WIN32
// clang-format off
#include <windows.h>
#include <consoleapi.h>
// clang-format on
#endif

namespace xpano::cli::windows {

// This is needed to redirect stdout to the console on Windows, because we are
// building with the WIN32 subsystem (app with no console window).

Attach::Attach() {
#ifdef _WIN32
  if (AttachConsole(ATTACH_PARENT_PROCESS)) {
    attached_console = true;
    freopen_s(&attached_stdout, "CONOUT$", "w", stdout);
  }
#endif
}

Attach::~Attach() {
#ifdef _WIN32
  if (attached_stdout) {
    fflush(attached_stdout);
    fclose(attached_stdout);
  }
  if (attached_console) {
    FreeConsole();
  }
#endif
}

}  // namespace xpano::cli::windows
