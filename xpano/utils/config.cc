#include "xpano/utils/config.h"

#include <filesystem>
#include <fstream>
#include <optional>

#include <spdlog/spdlog.h>

#include "xpano/constants.h"

namespace xpano::utils::config {
Config Load(std::optional<std::filesystem::path> app_data_path) {
  if (!app_data_path) {
    spdlog::warn("No app data path provided, using default config");
    return {};
  }

  Config config;
  std::filesystem::path config_path = *app_data_path / kConfigFilename;
  std::ifstream config_file(config_path);
  if (!config_file) {
    spdlog::warn("Failed to open config file, using default config");
    return {};
  }

  if (!(config_file >> config.window_width >> config.window_height)) {
    spdlog::warn("Failed to read config file, using default config");
    return {};
  }

  if (config.window_width < kMinWindowSize ||
      config.window_height < kMinWindowSize) {
    spdlog::warn(
        "Config file contains invalid window size, using default config");
    return {};
  }

  return config;
}

void Save(std::optional<std::filesystem::path> app_data_path,
          const Config& config) {
  if (!app_data_path) {
    spdlog::warn("No app data path provided, not saving config");
    return;
  }

  std::filesystem::path config_path = *app_data_path / kConfigFilename;
  std::ofstream config_file(config_path);
  if (!config_file) {
    spdlog::warn("Failed to open config file for writing");
    return;
  }

  if (!(config_file << config.window_width << " " << config.window_height)) {
    spdlog::warn("Failed to write config file");
  }
}

}  // namespace xpano::utils::config
