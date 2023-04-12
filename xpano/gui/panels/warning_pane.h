#pragma once

#include <queue>
#include <unordered_set>

#include "xpano/gui/action.h"

namespace xpano::gui {

class WarningPane {
 public:
  void Draw();
  void Queue(Action action);

 private:
  void Show(ActionType action);

  ActionType current_action_type_ = ActionType::kNone;

  std::queue<ActionType> pending_warnings_;
  std::unordered_set<ActionType> dont_show_again_;
};

}  // namespace xpano::gui
