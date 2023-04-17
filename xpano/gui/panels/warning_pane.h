#pragma once

#include <optional>
#include <queue>
#include <string>
#include <unordered_set>

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
  void Draw();
  void DrawExtra(WarningType warning);
  void Queue(WarningType warning);
  void QueueNewVersion(version::Triplet previous_version,
                       std::optional<utils::Text> changelog);

 private:
  void Show(WarningType warning);

  WarningType current_warning_ = WarningType::kNone;

  std::queue<WarningType> pending_warnings_;
  std::unordered_set<WarningType> dont_show_again_;

  std::string new_version_message_;
  std::optional<utils::Text> changelog_;
};

}  // namespace xpano::gui
