// SPDX-FileCopyrightText: 2026 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xpano/gui/panels/shortcuts_pane.h"

#include <array>
#include <span>

#include <imgui.h>

#include "xpano/gui/shortcut.h"

namespace xpano::gui {

namespace {

struct ShortcutRow {
  const char* action;
  const char* input;
};

void DrawShortcutTable(const char* table_id,
                       std::span<const ShortcutRow> rows) {
  if (ImGui::BeginTable(
          table_id, 2,
          ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg)) {
    for (const auto& row : rows) {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::TextUnformatted(row.action);
      ImGui::TableNextColumn();
      ImGui::TextDisabled("%s", row.input);
    }
    ImGui::EndTable();
  }
}

constexpr auto kKeyboardShortcuts = std::array{
    ShortcutRow{"Open files", Label(ShortcutType::kOpen)},
    ShortcutRow{"Export", Label(ShortcutType::kExport)},
    ShortcutRow{"Toggle adjustments", Label(ShortcutType::kRotate)},
    ShortcutRow{"Toggle crop", Label(ShortcutType::kCrop)},
    ShortcutRow{"Reset options", Label(ShortcutType::kReset)},
    ShortcutRow{"Increase UI scale", Label(ShortcutType::kScaleUp)},
    ShortcutRow{"Decrease UI scale", Label(ShortcutType::kScaleDown)},
    ShortcutRow{"Show debug info", Label(ShortcutType::kDebug)},
};

constexpr auto kMouseShortcuts = std::array{
    ShortcutRow{"Add/remove a thumbnail in the selected panorama",
                Label(ShortcutType::kModifyPano)},
    ShortcutRow{"Zoom the preview", "MOUSE WHEEL"},
    ShortcutRow{"Pan the zoomed preview", "DRAG"},
    ShortcutRow{"Scroll the thumbnails", "MOUSE WHEEL"},
};

}  // namespace

void ShortcutsPane::Show() { show_ = true; }

void ShortcutsPane::Draw() {
  if (!show_) {
    return;
  }

  const ImGuiWindowFlags window_flags =
      ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize;

  ImGui::Begin("Keyboard shortcuts", &show_, window_flags);

  ImGui::SeparatorText("Keyboard");
  DrawShortcutTable("keyboard_shortcuts", kKeyboardShortcuts);
  ImGui::SeparatorText("Mouse");
  DrawShortcutTable("mouse_shortcuts", kMouseShortcuts);

  ImGui::End();
}

}  // namespace xpano::gui
