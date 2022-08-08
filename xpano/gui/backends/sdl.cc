#include "gui/backends/sdl.h"

#include <imgui.h>
#include <opencv2/core.hpp>
#include <SDL.h>

#include "gui/backends/base.h"
#include "utils/vec.h"
#include "utils/vec_converters.h"

namespace xpano::gui::backends {

Sdl::Sdl(SDL_Renderer *renderer) : renderer_(renderer) {}

Texture Sdl::CreateTexture(utils::Vec2i size) {
  auto *sdl_tex = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_BGR24,
                                    SDL_TEXTUREACCESS_STATIC, size[0], size[1]);
  return {static_cast<ImTextureID>(sdl_tex), TexDeleter{this}};
}

void Sdl::UpdateTexture(ImTextureID tex, cv::Mat image) {
  auto target = utils::SdlRect(utils::Point2i{0}, utils::ToIntVec(image.size));
  auto *sdl_tex = static_cast<SDL_Texture *>(tex);
  SDL_UpdateTexture(sdl_tex, &target, image.data,
                    static_cast<int>(image.step1()));
}

void Sdl::DestroyTexture(ImTextureID tex) {
  SDL_DestroyTexture(static_cast<SDL_Texture *>(tex));
}

}  // namespace xpano::gui::backends
