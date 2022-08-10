#pragma once

#include <string>

namespace xpano::utils::imgui {

void ReloadFont(const std::string& font_path, float scale);

void InfoMarker(const char* desc);

}  // namespace xpano::utils::imgui
