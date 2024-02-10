// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <optional>

#include "xpano/constants.h"
#include "xpano/pipeline/options.h"
#include "xpano/utils/sdl_.h"
#include "xpano/utils/serialize.h"
#include "xpano/version.h"

namespace xpano::utils::config {

using LoadingStatus = utils::serialize::DeserializeStatus;

struct AppState {
  int window_width = kWindowWidth;
  int window_height = kWindowHeight;
  int pipeline_options_version = pipeline::kOptionsVersion;
  version::Triplet xpano_version = version::Current();
};

struct Config {
  AppState app_state;
  LoadingStatus user_options_status;
  pipeline::Options user_options;
};

Config Load(std::optional<std::filesystem::path> app_data_path);

void Save(std::optional<std::filesystem::path> app_data_path,
          utils::sdl::WindowSize window_size, const pipeline::Options& options);

}  // namespace xpano::utils::config
