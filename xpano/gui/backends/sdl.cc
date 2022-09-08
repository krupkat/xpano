#include "xpano/gui/backends/sdl.h"

#include <imgui.h>
#include <opencv2/core.hpp>
#include <SDL.h>
#include <spdlog/spdlog.h>

#include "xpano/gui/backends/base.h"
#include "xpano/utils/vec.h"
#include "xpano/utils/vec_converters.h"

namespace xpano::gui::backends {

Sdl::Sdl(SDL_Renderer *renderer) : renderer_(renderer) {
  if (SDL_GetRendererInfo(renderer, &info_) == 0) {
    spdlog::info("Current SDL_Renderer: {}", info_.name);
    spdlog::info("Max tex width: {}", info_.max_texture_width);
    spdlog::info("Max tex height: {}", info_.max_texture_height);
  } else {
    spdlog::error("Failed to get SDL_RendererInfo: {}", SDL_GetError());
  }
}

Texture Sdl::CreateTexture(utils::Vec2i size) {
  if (size[0] > info_.max_texture_width || size[1] > info_.max_texture_height) {
    spdlog::error("Texture size {} x {} is too big.", size[0], size[1]);
    return nullptr;
  }
  const char *old_texture_sampling = SDL_GetHint(SDL_HINT_RENDER_SCALE_QUALITY);
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");
  auto *sdl_tex = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_BGR24,
                                    SDL_TEXTUREACCESS_STATIC, size[0], size[1]);
  if (old_texture_sampling) {
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, old_texture_sampling);
  }
  if (sdl_tex == nullptr) {
    spdlog::error("Failed to create SDL_Texture: {}", SDL_GetError());
    return nullptr;
  }
  return {static_cast<ImTextureID>(sdl_tex), TexDeleter{this}};
}

void Sdl::UpdateTexture(ImTextureID tex, cv::Mat image) {
  auto target = utils::SdlRect(utils::Point2i{0}, utils::ToIntVec(image.size));
  auto *sdl_tex = static_cast<SDL_Texture *>(tex);
  if (SDL_UpdateTexture(sdl_tex, &target, image.data,
                        static_cast<int>(image.step1())) != 0) {
    spdlog::error("Failed to update SDL_Texture: {}", SDL_GetError());
  }
}

void Sdl::DestroyTexture(ImTextureID tex) {
  SDL_DestroyTexture(static_cast<SDL_Texture *>(tex));
}

}  // namespace xpano::gui::backends
