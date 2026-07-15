// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <imgui.h>
#include <opencv2/core.hpp>

#include "xpano/utils/vec.h"

namespace xpano::gui::backends {

class Base;

class Texture {
 public:
  Texture() = default;
  Texture(ImTextureID tex, Base* backend);
  Texture(const Texture&) = delete;
  Texture(Texture&& other) noexcept;
  Texture& operator=(const Texture&) = delete;
  Texture& operator=(Texture&& other) noexcept;
  ~Texture();

  explicit operator bool() const noexcept;

  [[nodiscard]] ImTextureID Get() const;

 private:
  ImTextureID tex_ = 0;
  Base* backend_ = nullptr;
};

class Base {
 public:
  virtual ~Base() = default;
  virtual Texture CreateTexture(utils::Vec2i size) = 0;
  virtual void UpdateTexture(ImTextureID tex, cv::Mat image) = 0;
  virtual void DestroyTexture(ImTextureID tex) = 0;
};

}  // namespace xpano::gui::backends
