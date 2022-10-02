#include "xpano/gui/panels/bugreport_pane.h"
#include <iostream>

#include <algorithm>
#include <iterator>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <imgui.h>

#include "xpano/constants.h"
#include "xpano/utils/future.h"
#include "xpano/utils/text.h"
#include "xpano/utils/sdl_.h"

namespace xpano::gui {

namespace {

const std::string kGithubIssuesLinkText =
    R"(https://github.com/krupkat/xpano/issues)";

const std::string kTomasEmailText =
    R"(tomas@krupkat.cz)";

}  // namespace

BugReportPane::BugReportPane(){}
  
void BugReportPane::Show() { show_ = true; }

void BugReportPane::Draw() {
  if (!show_) {
    return;
  }

  ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking |
                                  ImGuiWindowFlags_NoCollapse |
                                  ImGuiWindowFlags_NoSavedSettings;

  const auto text_base_width = ImGui::CalcTextSize("A").x;
  ImGui::SetNextWindowSize(ImVec2(kBugReportBoxWidth * text_base_width,
                                  kBugReportBoxHeight * ImGui::GetTextLineHeight()),
                           ImGuiCond_Once);
  ImGui::Begin("Report a bug", &show_, window_flags);

  ImGui::Text("Report bugs here: ");
  ImGui::Text(kGithubIssuesLinkText.c_str());

  if (ImGui::Button("Copy link to clipboard")) {
      ImGui::SetClipboardText(kGithubIssuesLinkText.c_str());
  }
  
  ImGui::Text("\n");
  
  ImGui::Text("You can also send an email to me at: ");
  ImGui::Text(kTomasEmailText.c_str());

  if (ImGui::Button("Copy email to clipboard")) {
      ImGui::SetClipboardText(kTomasEmailText.c_str());
  }

  auto log_file_path = *(xpano::utils::sdl::InitializePrefPath());
  ImGui::Text("\n\nThe log file directory is located at: \n");
  ImGui::Text(log_file_path.string().c_str());
  if (ImGui::Button("Copy path to clipboard")) {
      ImGui::SetClipboardText(log_file_path.string().c_str());
  }


  ImGui::End();
}

}  // namespace xpano::gui
