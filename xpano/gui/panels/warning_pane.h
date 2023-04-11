#pragma once

#include <unordered_set>

#include "xpano/gui/action.h"

namespace xpano::gui {

class WarningPane {
 public:
  void Draw();
  void Show(Action action);

 private:
  ActionType current_action_type_ = ActionType::kNone;
  std::unordered_set<ActionType> dont_show_again_;
};

}  // namespace xpano::gui
