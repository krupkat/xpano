// SPDX-FileCopyrightText: 2026 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "xpano/gui/file_dialog/base.h"

struct SDL_Window;

namespace xpano::gui::file_dialog {

class Sdl final : public Base {
 public:
  explicit Sdl(SDL_Window* window);

  std::future<
      std::expected<std::vector<std::filesystem::path>, file_dialog::Error>>
  Open(const Action& action) override;
  std::future<std::expected<std::filesystem::path, file_dialog::Error>> Save(
      const std::string& default_name) override;

 private:
  SDL_Window* window_;
};

}  // namespace xpano::gui::file_dialog
