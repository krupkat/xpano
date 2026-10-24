// SPDX-FileCopyrightText: 2026 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

namespace xpano::gui {

class ShortcutsPane {
 public:
  void Draw();
  void Show();

 private:
  bool show_ = false;
};

}  // namespace xpano::gui
