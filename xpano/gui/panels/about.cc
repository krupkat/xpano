#include "gui/panels/about.h"

#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <imgui.h>

#include "constants.h"
#include "utils/future.h"
#include "utils/text.h"

namespace xpano::gui {

namespace {

const std::string kAboutText =
    R"(Here you can check out the full app changelog, licenses of the
libraries used in Xpano as well as the full terms of the GPL license
under which this app is distributed.

=============

This software is based in part on the work of the Independent JPEG Group.

=============

Xpano - a tool for stitching photos into panoramas.
Copyright (C) 2022  Tomas Krupka

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.)";

utils::Text DefaultNotice() {
  auto text = utils::Text{.name = "Readme", .lines = {}};
  std::istringstream stream(kAboutText);
  for (std::string line; std::getline(stream, line);) {
    text.lines.push_back(line);
  }
  return text;
}
}  // namespace

AboutPane::AboutPane(std::future<std::vector<utils::Text>> licenses)
    : licenses_future_(std::move(licenses)), licenses_{DefaultNotice()} {}

void AboutPane::Show() { show_ = true; }

void AboutPane::Draw() {
  if (!show_) {
    return;
  }

  ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking |
                                  ImGuiWindowFlags_NoCollapse |
                                  ImGuiWindowFlags_NoSavedSettings;

  const auto text_base_width = ImGui::CalcTextSize("A").x;
  ImGui::SetNextWindowSize(ImVec2(kAboutBoxWidth * text_base_width,
                                  kAboutBoxHeight * ImGui::GetTextLineHeight()),
                           ImGuiCond_Once);
  ImGui::Begin("About", &show_, window_flags);

  ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

  if (licenses_.size() == 1 &&
      xpano::utils::future::IsReady(licenses_future_)) {
    auto temp_licenses_ = licenses_future_.get();
    std::copy(temp_licenses_.begin(), temp_licenses_.end(),
              std::back_inserter(licenses_));
  }

  if (ImGui::BeginCombo("##license_combo",
                        licenses_[current_license_].name.c_str())) {
    for (int i = 0; i < licenses_.size(); ++i) {
      if (ImGui::Selectable(licenses_[i].name.c_str(), current_license_ == i)) {
        current_license_ = i;
      }
    }
    ImGui::EndCombo();
  }

  ImGui::BeginChild("License", ImVec2(0, 0));
  const auto& license = licenses_[current_license_];

  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
  ImGuiListClipper clipper;
  int num_lines = static_cast<int>(license.lines.size());
  clipper.Begin(num_lines);
  while (clipper.Step()) {
    for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
      ImGui::TextUnformatted(license.lines[i].c_str());
    }
  }
  ImGui::PopStyleVar();
  ImGui::EndChild();

  ImGui::End();
}

}  // namespace xpano::gui
