#include "xpano/gui/panels/bugreport_pane.h"

#include <string>

#include <imgui.h>

#include "xpano/constants.h"

namespace xpano::gui {

namespace {

const std::string kGithubIssuesLink =
    R"(https://github.com/krupkat/xpano/issues)";

const std::string kAuthorEmail = R"(tomas@krupkat.cz)";

}  // namespace

BugReportPane::BugReportPane(logger::Logger *logger) : logger_(logger) {}

void BugReportPane::Show() { show_ = true; }
void BugReportPane::Draw() {
  if (!show_) {
    return;
  }

  ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking |
                                  ImGuiWindowFlags_NoCollapse |
                                  ImGuiWindowFlags_NoSavedSettings;

  ImGui::Begin("Report a bug", &show_, window_flags);

  ImGui::Text("Report bugs here: ");
  ImGui::TextUnformatted(kGithubIssuesLink.c_str());

  if (ImGui::Button("Copy link to clipboard")) {
    ImGui::SetClipboardText(kGithubIssuesLink.c_str());
  }

  ImGui::Text("\n");

  ImGui::Text("You can also send the bug report to my email: ");
  ImGui::TextUnformatted(kAuthorEmail.c_str());

  if (ImGui::Button("Copy email to clipboard")) {
    ImGui::SetClipboardText(kAuthorEmail.c_str());
  }

  ImGui::Text("\n\nThe log file directory is located at: \n");
  if (auto log_path = logger_->GetLogDirPath(); log_path) {
    ImGui::TextUnformatted(log_path->c_str());
    if (ImGui::Button("Copy path to clipboard")) {
      ImGui::SetClipboardText(log_path->c_str());
    }
  } else {
    ImGui::Text("Could not initialize log file directory");
  }

  ImGui::End();
}

}  // namespace xpano::gui
