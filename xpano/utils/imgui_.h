#pragma once

#include <string>

#include <imgui.h>

namespace xpano::utils::imgui {

class FontLoader {
 public:
  FontLoader(std::string alphabet_font_path, std::string symbols_font_path);

  void Reload(float scale);

 private:
  void ComputeGlyphRanges();

  std::string alphabet_font_path_;
  std::string symbols_font_path_;
  ImVector<ImWchar> alphabet_ranges_;
  ImVector<ImWchar> symbol_ranges_;
};

void InfoMarker(const std::string& label, const std::string& desc);

}  // namespace xpano::utils::imgui
