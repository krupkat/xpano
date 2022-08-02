#pragma once

namespace xpano::gui {

enum class ActionType {
  kNone,
  kOpenFiles,
  kShowImage,
  kShowMatch,
  kShowPano,
  kModifyPano,
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
