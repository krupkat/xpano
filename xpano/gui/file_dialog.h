#pragma once

#include <optional>
#include <string>
#include <vector>

#include "gui/action.h"

namespace xpano::gui::file_dialog {

std::vector<std::string> Open(Action action);

std::optional<std::string> Save();

}  // namespace xpano::gui::file_dialog
