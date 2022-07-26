#pragma once

#include <optional>
#include <string>
#include <vector>

#include "xpano/gui/action.h"

namespace xpano::gui::file_dialog {

std::vector<std::string> Open(Action action);

std::optional<std::string> Save(const std::string& default_name);

}  // namespace xpano::gui::file_dialog
