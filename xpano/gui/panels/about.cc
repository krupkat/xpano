#include "xpano/gui/panels/about.h"

#include <algorithm>
#include <iterator>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <imgui.h>

#include "xpano/constants.h"
#include "xpano/utils/future.h"
#include "xpano/utils/imgui_.h"
#include "xpano/utils/text.h"

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

AboutPane::AboutPane(std::future<utils::Texts> licenses)
    : licenses_future_(std::move(licenses)), licenses_{DefaultNotice()} {}

void AboutPane::Show() { show_ = true; }

std::optional<utils::Text> AboutPane::GetText(const std::string& name) {
  if (licenses_future_.valid()) {
    WaitForLicenseLoading();
  }
  auto result = std::find_if(
      licenses_.begin(), licenses_.end(),
      [&name](const utils::Text& text) { return text.name == name; });
  if (result == licenses_.end()) {
    return {};
  }
  return *result;
}

void AboutPane::WaitForLicenseLoading() {
  auto temp_licenses = licenses_future_.get();
  std::copy(temp_licenses.begin(), temp_licenses.end(),
            std::back_inserter(licenses_));
}

void AboutPane::Draw() {
  if (!show_) {
    return;
  }

  if (licenses_.size() == 1 &&
      xpano::utils::future::IsReady(licenses_future_)) {
    WaitForLicenseLoading();
  }

  ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking |
                                  ImGuiWindowFlags_NoCollapse |
                                  ImGuiWindowFlags_NoSavedSettings;

  ImGui::SetNextWindowSize(
      utils::imgui::DpiAwareSize(kAboutBoxWidth, kAboutBoxHeight),
      ImGuiCond_Once);
  ImGui::Begin("About", &show_, window_flags);

  ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
  if (ImGui::BeginCombo("##license_combo",
                        licenses_[current_license_].name.c_str())) {
    for (int i = 0; i < licenses_.size(); ++i) {
      if (ImGui::Selectable(licenses_[i].name.c_str(), current_license_ == i)) {
        current_license_ = i;
      }
    }
    ImGui::EndCombo();
  }

  utils::imgui::DrawScrollableText("License",
                                   licenses_[current_license_].lines);

  ImGui::End();
}

}  // namespace xpano::gui
