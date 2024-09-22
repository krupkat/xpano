// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-FileCopyrightText: 2022 Naachiket Pant
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xpano/gui/panels/bugreport_pane.h"

#include <string>

#include <imgui.h>

#include "xpano/constants.h"
#include "xpano/log/logger.h"

namespace xpano::gui {

BugReportPane::BugReportPane(logger::Logger *logger) : logger_(logger) {}

void BugReportPane::Show() { show_ = true; }
void BugReportPane::Draw() {
  if (!show_) {
    return;
  }

  const ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking |
                                        ImGuiWindowFlags_NoCollapse |
                                        ImGuiWindowFlags_NoSavedSettings;

  ImGui::Begin("Support", &show_, window_flags);

  ImGui::Text("Please file any issues or feature requests on GitHub:");
  ImGui::TextUnformatted(kGithubIssuesLink.c_str());

  if (ImGui::Button("Copy link to clipboard")) {
    ImGui::SetClipboardText(kGithubIssuesLink.c_str());
  }

  ImGui::Text("\n");

  ImGui::Text("You can also contact me directly through e-mail:");
  ImGui::TextUnformatted(kAuthorEmail.c_str());

  if (ImGui::Button("Copy e-mail to clipboard")) {
    ImGui::SetClipboardText(kAuthorEmail.c_str());
  }

  ImGui::Text("\n\nDebug logs are located in:\n");
  if (auto log_path = logger_->GetLogDirPath(); log_path) {
    ImGui::TextUnformatted(log_path->c_str());
    if (ImGui::Button("Copy path to clipboard")) {
      ImGui::SetClipboardText(log_path->c_str());
    }
  } else {
    ImGui::Text("Could not initialize a log file directory");
  }

  ImGui::End();
}

}  // namespace xpano::gui
