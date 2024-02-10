// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xpano/utils/config.h"

#include <filesystem>
#include <optional>

#include <spdlog/spdlog.h>

#include "xpano/constants.h"
#include "xpano/pipeline/options.h"
#include "xpano/utils/sdl_.h"
#include "xpano/utils/serialize.h"

namespace xpano::utils::config {

Config Load(std::optional<std::filesystem::path> app_data_path) {
  if (!app_data_path) {
    spdlog::warn("No app data path provided, using default config");
    return {};
  }

  Config config;
  auto [app_state_status, app_state] =
      utils::serialize::DeserializeWithVersion<AppState>(*app_data_path /
                                                         kAppConfigFilename);

  if (app_state_status != LoadingStatus::kSuccess) {
    spdlog::warn("Reverting to app state defaults");
  }

  config.app_state = app_state;

  if (config.app_state.pipeline_options_version != pipeline::kOptionsVersion) {
    spdlog::warn("Version mismatch, reverting to user options defaults");
    config.user_options_status = LoadingStatus::kBreakingChange;
    return config;
  }

  auto [user_options_status, user_options] =
      utils::serialize::DeserializeWithVersion<pipeline::Options>(
          *app_data_path / kUserConfigFilename);

  if (user_options_status != LoadingStatus::kSuccess) {
    spdlog::warn("Reverting to user options defaults");
  }

  config.user_options_status = user_options_status;
  config.user_options = user_options;
  return config;
}

void Save(std::optional<std::filesystem::path> app_data_path,
          utils::sdl::WindowSize window_size,
          const pipeline::Options& options) {
  if (!app_data_path) {
    spdlog::warn("No app data path provided, not saving config");
    return;
  }

  auto error = utils::serialize::SerializeWithVersion(
      *app_data_path / kAppConfigFilename,
      AppState{.window_width = window_size.width,
               .window_height = window_size.height});

  if (error) {
    spdlog::warn("Failed to save app state.");
  }

  error = utils::serialize::SerializeWithVersion(
      *app_data_path / kUserConfigFilename, options);

  if (error) {
    spdlog::warn("Failed to save user options.");
  }
}

}  // namespace xpano::utils::config
