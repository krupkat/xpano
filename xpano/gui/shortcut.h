// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xpano/gui/action.h"

namespace xpano::gui {

enum class ShortcutType { kOpen, kExport, kDebug, kReset, kRotate, kCrop };

const char* Label(ShortcutType type);

Action CheckKeybindings();

}  // namespace xpano::gui
