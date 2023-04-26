#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "xpano/gui/action.h"

namespace xpano::gui::file_dialog {

std::vector<std::filesystem::path> Open(const Action& action);

std::optional<std::filesystem::path> Save(const std::string& default_name);

}  // namespace xpano::gui::file_dialog
