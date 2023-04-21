#pragma once

#include <filesystem>
#include <iterator>
#include <variant>
#include <vector>

namespace xpano::gui {

enum class ActionType {
  kNone,
  kCancelPipeline,
  kToggleCrop,
  kDisableHighlight,
  kExport,
  kInpaint,
  kLoadFiles,
  kOpenDirectory,
  kOpenFiles,
  kShowAbout,
  kShowBugReport,
  kShowImage,
  kShowMatch,
  kShowPano,
  kModifyPano,
  kRecomputePano,
  kQuit,
  kToggleDebugLog,
  kWarnInputConversion,
  kResetOptions
};

struct ShowPanoExtra {
  bool full_res = false;
  bool scroll_thumbnails = false;
};

using LoadFilesExtra = std::vector<std::filesystem::path>;

struct Action {
  ActionType type = ActionType::kNone;
  int target_id;
  bool delayed = false;
  std::variant<ShowPanoExtra, LoadFilesExtra> extra;
};

template <typename TExtraType>
TExtraType ValueOrDefault(const Action& action) {
  if (const auto* extra = std::get_if<TExtraType>(&action.extra); extra) {
    return *extra;
  }
  return {};
}

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
    lhs.extra = rhs.extra;
  }
  return lhs;
}

}  // namespace xpano::gui
