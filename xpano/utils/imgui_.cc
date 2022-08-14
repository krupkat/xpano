#include "imgui_.h"

#include <cmath>
#include <filesystem>
#include <optional>
#include <utility>

#include <imgui.h>
#include <imgui_impl_sdlrenderer.h>
#include <spdlog/spdlog.h>

#include "constants.h"

namespace xpano::utils::imgui {

namespace {

std::optional<std::string> FindFont(const std::string& executable_path,
                                    const std::string& font_rel_path) {
  std::filesystem::path base =
      std::filesystem::path(executable_path).parent_path();
  auto font_path = base / font_rel_path;
  if (std::filesystem::exists(font_path)) {
    return font_path.string();
  }

  const auto linux_prefix = std::filesystem::path("../share");
  auto linux_font_path = base / linux_prefix / font_rel_path;
  if (std::filesystem::exists(linux_font_path)) {
    return linux_font_path.string();
  }

  spdlog::error("Couldn't find font: {}", font_rel_path);
  return {};
}

}  // namespace

FontLoader::FontLoader(std::string alphabet_font_path,
                       std::string symbols_font_path)
    : alphabet_font_path_(std::move(alphabet_font_path)),
      symbols_font_path_(std::move(symbols_font_path)) {}

void FontLoader::ComputeGlyphRanges() {
  ImFontGlyphRangesBuilder builder;
  builder.AddRanges(ImGui::GetIO().Fonts->GetGlyphRangesDefault());
  builder.BuildRanges(&alphabet_ranges_);

  builder.Clear();
  builder.AddText(kCheckMark);
  builder.BuildRanges(&symbol_ranges_);
}

bool FontLoader::Init(std::string executable_path) {
  ComputeGlyphRanges();

  if (auto font = FindFont(executable_path, alphabet_font_path_); font) {
    alphabet_font_path_ = *font;
  } else {
    return false;
  }

  if (auto font = FindFont(executable_path, symbols_font_path_); font) {
    symbols_font_path_ = *font;
  } else {
    return false;
  }

  return true;
}

void FontLoader::Reload(float scale) {
  ImGuiIO& imgui_io = ImGui::GetIO();
  imgui_io.Fonts->Clear();
  imgui_io.Fonts->AddFontFromFileTTF(alphabet_font_path_.c_str(),
                                     std::roundf(18.0f * scale), nullptr,
                                     alphabet_ranges_.Data);
  ImFontConfig config;
  config.MergeMode = true;
  imgui_io.Fonts->AddFontFromFileTTF(symbols_font_path_.c_str(),
                                     std::roundf(18.0f * scale), &config,
                                     symbol_ranges_.Data);

  ImGui_ImplSDLRenderer_DestroyDeviceObjects();
  ImGui_ImplSDLRenderer_CreateDeviceObjects();

  ImGui::GetStyle() = {};
  ImGui::GetStyle().ScaleAllSizes(scale);
}

void InfoMarker(const std::string& label, const std::string& desc) {
  ImGui::TextDisabled(label.c_str());
  if (ImGui::IsItemHovered()) {
    ImGui::BeginTooltip();
    ImGui::TextUnformatted(desc.c_str());
    ImGui::EndTooltip();
  }
}

}  // namespace xpano::utils::imgui
