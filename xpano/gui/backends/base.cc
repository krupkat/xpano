// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xpano/gui/backends/base.h"

#include <imgui.h>

namespace xpano::gui::backends {

Texture::Texture(ImTextureID tex, Base* backend)
    : tex_(tex), backend_(backend) {}

Texture::Texture(Texture&& other) noexcept
    : tex_(other.tex_), backend_(other.backend_) {
  other.backend_ = nullptr;
}

Texture& Texture::operator=(Texture&& other) noexcept {
  if (this != &other) {
    if (backend_ != nullptr) {
      backend_->DestroyTexture(tex_);
    }
    tex_ = other.tex_;
    backend_ = other.backend_;
    other.backend_ = nullptr;
  }
  return *this;
}

Texture::~Texture() {
  if (backend_ != nullptr) {
    backend_->DestroyTexture(tex_);
  }
}

Texture::operator bool() const noexcept { return tex_ != 0; }

ImTextureID Texture::Get() const { return tex_; }

}  // namespace xpano::gui::backends
