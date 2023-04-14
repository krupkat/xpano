#pragma once

#include <queue>
#include <unordered_set>

#include "xpano/gui/action.h"

namespace xpano::gui {

enum class WarningType {
  kNone,
  kWarnInputConversion,
  kFirstTimeLaunch,
  kUserPrefBreakingChange,
  kUserPrefCouldntLoad
};

class WarningPane {
 public:
  void Draw();
  void Queue(WarningType warning);

 private:
  void Show(WarningType warning);

  WarningType current_warning_ = WarningType::kNone;

  std::queue<WarningType> pending_warnings_;
  std::unordered_set<WarningType> dont_show_again_;
};

}  // namespace xpano::gui
