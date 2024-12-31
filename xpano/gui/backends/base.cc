// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xpano/gui/backends/base.h"

#include <imgui.h>

namespace xpano::gui::backends {

TexDeleter::TexDeleter(Base* backend) : backend_(backend) {};

void TexDeleter::operator()(ImTextureID tex) {
  if (backend_ != nullptr) {
    backend_->DestroyTexture(tex);
  }
}

}  // namespace xpano::gui::backends
