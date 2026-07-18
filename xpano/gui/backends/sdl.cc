// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xpano/gui/backends/sdl.h"

#include <imgui.h>
#include <opencv2/core.hpp>
#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>

#include "xpano/gui/backends/base.h"
#include "xpano/utils/vec.h"
#include "xpano/utils/vec_converters.h"

namespace xpano::gui::backends {

Sdl::Sdl(SDL_Renderer* renderer) : renderer_(renderer) {
  if (const char* name = SDL_GetRendererName(renderer_)) {
    spdlog::info("Current SDL_Renderer: {}", name);
  } else {
    spdlog::error("Failed to get SDL_RendererInfo: {}", SDL_GetError());
  }

  if (SDL_PropertiesID props = SDL_GetRendererProperties(renderer)) {
    max_texture_size_ = static_cast<int>(SDL_GetNumberProperty(
        props, SDL_PROP_RENDERER_MAX_TEXTURE_SIZE_NUMBER, 0));
    spdlog::info("Max texture size: {}", max_texture_size_);
  } else {
    spdlog::error("Failed to get SDL_RendererProperties: {}", SDL_GetError());
  }
}

Texture Sdl::CreateTexture(utils::Vec2i size) {
  if (size[0] > max_texture_size_ || size[1] > max_texture_size_) {
    spdlog::error("Texture size {} x {} is too big.", size[0], size[1]);
    return {};
  }
  auto* sdl_tex = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_BGR24,
                                    SDL_TEXTUREACCESS_STATIC, size[0], size[1]);
  if (sdl_tex == nullptr) {
    spdlog::error("Failed to create SDL_Texture: {}", SDL_GetError());
    return {};
  }
  return {static_cast<ImTextureID>(reinterpret_cast<intptr_t>(sdl_tex)), this};
}

void Sdl::UpdateTexture(ImTextureID tex, cv::Mat image) {
  auto target = utils::SdlRect(utils::Point2i{0}, utils::ToIntVec(image.size));
  // NOLINTNEXTLINE(performance-no-int-to-ptr): taken from imgui faq
  auto* sdl_tex = reinterpret_cast<SDL_Texture*>(static_cast<intptr_t>(tex));
  if (!SDL_UpdateTexture(sdl_tex, &target, image.data,
                         static_cast<int>(image.step1()))) {
    spdlog::error("Failed to update SDL_Texture: {}", SDL_GetError());
  }
}

void Sdl::DestroyTexture(ImTextureID tex) noexcept {
  SDL_DestroyTexture(
      // NOLINTNEXTLINE(performance-no-int-to-ptr): taken from imgui faq
      reinterpret_cast<SDL_Texture*>(static_cast<intptr_t>(tex)));
}

}  // namespace xpano::gui::backends
