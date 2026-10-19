// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>

#include "xpano/gui/action.h"

namespace xpano::gui {

enum class ShortcutType : std::uint8_t {
  kOpen,
  kExport,
  kDebug,
  kReset,
  kRotate,
  kCrop,
  kScaleUp,
  kScaleDown,
  kModifyPano
};

constexpr const char* Label(ShortcutType type) {
#ifdef __APPLE__
  switch (type) {
    case ShortcutType::kOpen:
      return "⌘ O";
    case ShortcutType::kExport:
      return "⌘ S";
    case ShortcutType::kDebug:
      return "⌘ D";
    case ShortcutType::kReset:
      return "⌘ R";
    case ShortcutType::kRotate:
      return "A";
    case ShortcutType::kCrop:
      return "C";
    case ShortcutType::kScaleUp:
      return "⌘ +";
    case ShortcutType::kScaleDown:
      return "⌘ -";
    case ShortcutType::kModifyPano:
      return "⌘ CLICK";
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
    case ShortcutType::kModifyPano:
      return "CTRL+CLICK";
    default:
      return "";
  }
#endif
}

Action CheckKeybindings();

}  // namespace xpano::gui
