#pragma once

#include <optional>
#include <queue>
#include <string>
#include <unordered_set>

#include "xpano/gui/action.h"
#include "xpano/gui/panels/about.h"
#include "xpano/utils/text.h"
#include "xpano/version.h"

namespace xpano::gui {

enum class WarningType {
  kNone,
  kWarnInputConversion,
  kFirstTimeLaunch,
  kUserPrefBreakingChange,
  kUserPrefCouldntLoad,
  kUserPrefResetOnRequest,
  kNewVersion
};

class WarningPane {
 public:
  WarningPane(AboutPane* about_pane) : about_pane_(about_pane) {}

  void Draw();
  void DrawExtra(WarningType warning);
  void Queue(WarningType warning);
  void QueueNewVersion(version::Triplet previous_version);

 private:
  void Show(WarningType warning);

  AboutPane* about_pane_;

  WarningType current_warning_ = WarningType::kNone;

  std::queue<WarningType> pending_warnings_;
  std::unordered_set<WarningType> dont_show_again_;

  std::string new_version_message_;
  std::optional<utils::Text> changelog_;
};

}  // namespace xpano::gui
