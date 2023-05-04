// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <type_traits>

#include <imgui.h>
#include <opencv2/core.hpp>

#include "xpano/utils/vec.h"

namespace xpano::gui::backends {

class Base;

class TexDeleter {
 public:
  TexDeleter() = default;
  explicit TexDeleter(Base* backend);
  void operator()(ImTextureID tex);

 private:
  Base* backend_ = nullptr;
};

using Texture = std::unique_ptr<std::remove_pointer_t<ImTextureID>, TexDeleter>;

class Base {
 public:
  virtual ~Base() = default;
  virtual Texture CreateTexture(utils::Vec2i size) = 0;
  virtual void UpdateTexture(ImTextureID tex, cv::Mat image) = 0;
  virtual void DestroyTexture(ImTextureID tex) = 0;
};

}  // namespace xpano::gui::backends
