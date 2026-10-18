// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xpano/gui/shortcut.h"

#include <imgui.h>

#include "xpano/gui/action.h"

namespace xpano::gui {

const char* Label(ShortcutType type) {
#ifdef __APPLE__
  switch (type) {
    case ShortcutType::kOpen:
      return reinterpret_cast<const char*>(u8"⌘ O");
    case ShortcutType::kExport:
      return reinterpret_cast<const char*>(u8"⌘ S");
    case ShortcutType::kDebug:
      return reinterpret_cast<const char*>(u8"⌘ D");
    case ShortcutType::kReset:
      return reinterpret_cast<const char*>(u8"⌘ R");
    case ShortcutType::kRotate:
      return "A";
    case ShortcutType::kCrop:
      return "C";
    case ShortcutType::kScaleUp:
      return reinterpret_cast<const char*>(u8"⌘ +");
    case ShortcutType::kScaleDown:
      return reinterpret_cast<const char*>(u8"⌘ -");
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
    case ShortcutType::kRotate:
      return "A";
    case ShortcutType::kCrop:
      return "C";
    case ShortcutType::kScaleUp:
      return "CTRL+PLUS";
    case ShortcutType::kScaleDown:
      return "CTRL+MINUS";
    default:
      return "";
  }
#endif
}

// On macOS, imgui swaps Cmd and Ctrl, so ImGuiMod_Ctrl means Cmd
Action CheckKeybindings() {
  constexpr ImGuiInputFlags kFlags = ImGuiInputFlags_RouteGlobal;
  if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O, kFlags)) {
    return {ActionType::kOpenFiles};
  }
  if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S, kFlags)) {
    return {ActionType::kExport};
  }
  if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_D, kFlags)) {
    return {ActionType::kToggleDebugLog};
  }
  if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_R, kFlags)) {
    return {ActionType::kResetOptions};
  }
  if (ImGui::Shortcut(ImGuiKey_A, kFlags)) {
    return {ActionType::kToggleRotate};
  }
  if (ImGui::Shortcut(ImGuiKey_C, kFlags)) {
    return {ActionType::kToggleCrop};
  }
  if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Equal, kFlags) ||
      ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Equal,
                      kFlags) ||
      ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_KeypadAdd, kFlags)) {
    return {ActionType::kIncreaseUiScale};
  }
  if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Minus, kFlags) ||
      ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_KeypadSubtract, kFlags)) {
    return {ActionType::kDecreaseUiScale};
  }
  return {ActionType::kNone};
}

}  // namespace xpano::gui
