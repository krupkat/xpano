#pragma once

#include <optional>
#include <queue>
#include <string>
#include <unordered_set>

#include "xpano/gui/file_dialog.h"
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
  kNewVersion,
  kFilePickerUnsupportedExt,
  kFilePickerUnknownError
};

struct Warning {
  WarningType type = WarningType::kNone;
  std::string extra_message = {};
};

class WarningPane {
 public:
  void Draw();
  void Queue(WarningType warning);
  void QueueNewVersion(version::Triplet previous_version,
                       std::optional<utils::Text> changelog);
  void QueueFilePickerError(const file_dialog::Error& error);

 private:
  void Show(Warning warning);
  void DrawExtra(const Warning& warning);

  Warning current_warning_ = {WarningType::kNone};

  std::queue<Warning> pending_warnings_;
  std::unordered_set<WarningType> dont_show_again_;

  std::optional<utils::Text> changelog_;
};

}  // namespace xpano::gui
