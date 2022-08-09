#pragma once

namespace xpano::gui {

enum class ActionType {
  kNone,
  kExport,
  kOpenDirectory,
  kOpenFiles,
  kShowImage,
  kShowMatch,
  kShowPano,
  kModifyPano,
  kQuit,
  kToggleDebugLog
};

struct Action {
  ActionType type = ActionType::kNone;
  int id;
};

inline Action& operator|=(Action& lhs, const Action& rhs) {
  if (rhs.type != ActionType::kNone) {
    lhs.type = rhs.type;
    lhs.id = rhs.id;
  }
  return lhs;
}

}  // namespace xpano::gui
