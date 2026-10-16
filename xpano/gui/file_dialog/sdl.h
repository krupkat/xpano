// SPDX-FileCopyrightText: 2026 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "xpano/gui/file_dialog/base.h"

struct SDL_Window;

namespace xpano::gui::file_dialog {

class Sdl final : public Base {
 public:
  explicit Sdl(SDL_Window* window);

  std::future<OpenResult> Open(const Action& action) override;
  std::future<SaveResult> Save(const std::string& default_name) override;

 private:
  SDL_Window* window_;
};

}  // namespace xpano::gui::file_dialog
