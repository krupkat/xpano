#pragma once

#include <iterator>
#include <vector>

namespace xpano::gui {

enum class ActionType {
  kNone,
  kCancelPipeline,
  kToggleCrop,
  kDisableHighlight,
  kExport,
  kInpaint,
  kOpenDirectory,
  kOpenFiles,
  kShowAbout,
  kShowBugReport,
  kShowImage,
  kShowMatch,
  kShowPano,
  kShowFullResPano,
  kModifyPano,
  kRecomputePano,
  kQuit,
  kToggleDebugLog,
  kWarnInputConversion
};

struct Action {
  ActionType type = ActionType::kNone;
  int target_id;
  bool delayed = false;
};

struct MultiAction {
  std::vector<Action> items;
};

inline MultiAction& operator|=(MultiAction& lhs, const Action& rhs) {
  if (rhs.type != ActionType::kNone) {
    lhs.items.push_back(rhs);
  }
  return lhs;
}

inline MultiAction& operator|=(MultiAction& lhs, const MultiAction& rhs) {
  std::copy(rhs.items.begin(), rhs.items.end(), std::back_inserter(lhs.items));
  return lhs;
}

inline Action RemoveDelay(Action action) {
  action.delayed = false;
  return action;
}

inline MultiAction ForwardDelayed(const MultiAction& actions) {
  MultiAction result;
  for (const auto& action : actions.items) {
    if (action.delayed) {
      result.items.push_back(RemoveDelay(action));
    }
  }
  return result;
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
