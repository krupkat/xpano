#pragma once

namespace xpano::gui {

enum class ActionType {
  kNone,
  kCancelPipeline,
  kDisableHighlight,
  kExport,
  kOpenDirectory,
  kOpenFiles,
  kShowAbout,
  kShowImage,
  kShowMatch,
  kShowPano,
  kModifyPano,
  kRecomputePano,
  kQuit,
  kToggleDebugLog
};

struct Action {
  ActionType type = ActionType::kNone;
  int target_id;
  bool delayed = false;
};

inline Action RemoveDelay(Action action) {
  action.delayed = false;
  return action;
}

inline Action& operator|=(Action& lhs, const Action& rhs) {
  if (rhs.type != ActionType::kNone) {
    lhs.type = rhs.type;
    lhs.target_id = rhs.target_id;
    lhs.delayed = rhs.delayed;
  }
  return lhs;
}

}  // namespace xpano::gui
