// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xpano/gui/shortcut.h"

#include <imgui.h>

#include "xpano/gui/action.h"

namespace xpano::gui {

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
  if (ImGui::Shortcut(ImGuiKey_F, kFlags)) {
    return {ActionType::kRecomputePanoFullRes};
  }
  if (ImGui::Shortcut(ImGuiKey_RightArrow, kFlags)) {
    return {ActionType::kNextSelection};
  }
  if (ImGui::Shortcut(ImGuiKey_LeftArrow, kFlags)) {
    return {ActionType::kPreviousSelection};
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
