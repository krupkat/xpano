#pragma once

#include <filesystem>
#include <optional>

#include "xpano/constants.h"
#include "xpano/pipeline/options.h"

namespace xpano::utils::config {

struct Config {
  int window_width = kWindowWidth;
  int window_height = kWindowHeight;
  pipeline::Options pipeline_options;
};

Config Load(std::optional<std::filesystem::path> app_data_path);

void Save(std::optional<std::filesystem::path> app_data_path,
          const Config& config);

}  // namespace xpano::utils::config
