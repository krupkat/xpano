#include "xpano/gui/panels/bugreport_pane.h"

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

namespace xpano::gui {

namespace {

const std::string kGithubIssuesLinkText =
    R"(Report bugs to: https://github.com/krupkat/xpano/issues)";

const std::string kTomasEmailText =
    R"(Or you can send an email to me: tomas@krupkat.cz)";

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

  ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

  ImGui::Text(kGithubIssuesLinkText.c_str());
  
  ImGui::Text("\n");
  
  ImGui::Text(kTomasEmailText.c_str());

  ImGui::End();
}

}  // namespace xpano::gui
