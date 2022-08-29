#include "gui/backends/sdl.h"

#include <imgui.h>
#include <opencv2/core.hpp>
#include <SDL.h>
#include <spdlog/spdlog.h>

#include "gui/backends/base.h"
#include "utils/vec.h"
#include "utils/vec_converters.h"

namespace xpano::gui::backends {

Sdl::Sdl(SDL_Renderer *renderer, SDL_Window *window)
    : renderer_(renderer), window_(window) {
  if (SDL_GetRendererInfo(renderer, &info_) == 0) {
    spdlog::info("Current SDL_Renderer: {}", info_.name);
    spdlog::info("Max tex width: {}", info_.max_texture_width);
    spdlog::info("Max tex height: {}", info_.max_texture_height);
  } else {
    spdlog::error("Failed to get SDL_RendererInfo: {}", SDL_GetError());
  }
  int num_displays = SDL_GetNumVideoDisplays();
  for (int i = 0; i < num_displays; i++) {
    float dpi;
    SDL_GetDisplayDPI(i, &dpi, nullptr, nullptr);
    spdlog::info("Display {}, dpi = {}", i, dpi);

    int num_modes = SDL_GetNumDisplayModes(i);
    for (int j = 0; j < num_modes; j++) {
      SDL_DisplayMode mode;
      if (SDL_GetDisplayMode(i, j, &mode) == 0) {
        spdlog::info("Display {}, mode {}, {}x{}@{}, {}", i, j, mode.w, mode.h,
                     mode.refresh_rate, SDL_GetPixelFormatName(mode.format));
      }
    }
  }
  SDL_DisplayMode mode;
  if (SDL_GetWindowDisplayMode(window, &mode) == 0) {
    spdlog::info("Selected mode, {}x{}@{}, {}", mode.w, mode.h,
                 mode.refresh_rate, SDL_GetPixelFormatName(mode.format));
  }

  int num_drivers = SDL_GetNumVideoDrivers();
  for (int i = 0; i < num_drivers; i++) {
    const char *video_driver = SDL_GetVideoDriver(i);
    spdlog::info("Video driver {}: {}", i, video_driver);
  }
  const char *video_driver = SDL_GetCurrentVideoDriver();
  spdlog::info("Selected: {}", video_driver);

  int rw, rh;
  SDL_GetRendererOutputSize(renderer, &rw, &rh);

  int ww, wh;
  SDL_GL_GetDrawableSize(window, &ww, &wh);

  int w, h;
  SDL_GetWindowSize(window, &w, &h);
  spdlog::info("Renderer: {}x{}", rw, rh);
  spdlog::info("GL Window: {}x{}", ww, wh);
  spdlog::info("Window: {}x{}", w, h);
}

Texture Sdl::CreateTexture(utils::Vec2i size) {
  if (size[0] > info_.max_texture_width || size[1] > info_.max_texture_height) {
    spdlog::error("Texture size {} x {} is too big.", size[0], size[1]);
    return nullptr;
  }
  auto *sdl_tex = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_BGR24,
                                    SDL_TEXTUREACCESS_STATIC, size[0], size[1]);
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
