#include "utils/resource.h"

#include <filesystem>
#include <optional>
#include <string>

#include <spdlog/spdlog.h>

namespace xpano::utils::resource {

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

}  // namespace xpano::utils::resource
