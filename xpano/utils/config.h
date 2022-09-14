#pragma once

#include <filesystem>
#include <optional>

#include "xpano/constants.h"

namespace xpano::utils::config {

struct Config {
  int window_width = kWindowWidth;
  int window_height = kWindowHeight;
};

Config Load(std::optional<std::filesystem::path> app_data_path);

void Save(std::optional<std::filesystem::path> app_data_path, Config config);

}  // namespace xpano::utils::config
