#include "utils/resource.h"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <SDL.h>
#include <spdlog/spdlog.h>

namespace xpano::utils::resource {

namespace {}  // namespace

std::optional<std::string> Find(const std::string& executable_path,
                                const std::string& rel_path) {
  std::filesystem::path base =
      std::filesystem::path(executable_path).parent_path();
  auto path = base / rel_path;
  if (std::filesystem::exists(path)) {
    return path.string();
  }

  const auto linux_prefix = std::filesystem::path("../share");
  auto linux_path = base / linux_prefix / rel_path;
  if (std::filesystem::exists(linux_path)) {
    return linux_path.string();
  }

  spdlog::warn("Couldn't find path: {}", rel_path);
  return {};
}

SdlSurface LoadIcon(const std::string& executable_path,
                    const std::string& path) {
  auto full_path = Find(executable_path, path);
  if (!full_path) {
    return {nullptr, &SDL_FreeSurface};
  }

  auto icon = cv::imread(*full_path, cv::IMREAD_UNCHANGED);
  if (icon.depth() != CV_8U || icon.channels() != 4) {
    spdlog::error("Icon is not RGBA");
    return {nullptr, &SDL_FreeSurface};
  }

  const Uint32 rmask = 0x00ff0000U;
  const Uint32 gmask = 0x0000ff00U;
  const Uint32 bmask = 0x000000ffU;
  const Uint32 amask = 0xff000000U;
  // this call doesn't allocate memory
  SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(
      icon.data, icon.cols, icon.rows, icon.channels() * 8,
      static_cast<int>(icon.step1()), rmask, gmask, bmask, amask);

  if (surface == nullptr) {
    spdlog::error("Failed to create SDL_Surface: {}", SDL_GetError());
    return {nullptr, &SDL_FreeSurface};
  }

  return {SDL_DuplicateSurface(surface), &SDL_FreeSurface};
}

}  // namespace xpano::utils::resource
