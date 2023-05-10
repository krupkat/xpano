// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xpano/gui/shortcut.h"

#include <imgui.h>

namespace xpano::gui {

const char* Label(ShortcutType type) {
#if defined(__APPLE__)
  switch (type) {
    case ShortcutType::kOpen:
      return reinterpret_cast<const char*>(u8"⌘ O");
    case ShortcutType::kExport:
      return reinterpret_cast<const char*>(u8"⌘ S");
    case ShortcutType::kDebug:
      return reinterpret_cast<const char*>(u8"⌘ D");
    case ShortcutType::kReset:
      return reinterpret_cast<const char*>(u8"⌘ R");
    default:
      return "";
  }
#else
  switch (type) {
    case ShortcutType::kOpen:
      return "CTRL+O";
    case ShortcutType::kExport:
      return "CTRL+S";
    case ShortcutType::kDebug:
      return "CTRL+D";
    case ShortcutType::kReset:
      return "CTRL+R";
    default:
      return "";
  }
#endif
}

Action CheckKeybindings() {
#if defined(__APPLE__)
  bool ctrl = ImGui::GetIO().KeySuper;
#else
  bool ctrl = ImGui::GetIO().KeyCtrl;
#endif
  if (ctrl && ImGui::IsKeyPressed(ImGuiKey_O)) {
    return {ActionType::kOpenFiles};
  }
  if (ctrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
    return {ActionType::kExport};
  }
  if (ctrl && ImGui::IsKeyPressed(ImGuiKey_D)) {
    return {ActionType::kToggleDebugLog};
  }
  if (ctrl && ImGui::IsKeyPressed(ImGuiKey_R)) {
    return {ActionType::kResetOptions};
  }
  return {ActionType::kNone};
}

}  // namespace xpano::gui
