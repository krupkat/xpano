#include "xpano/gui/panels/warning_pane.h"

#include <imgui.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

#include "xpano/constants.h"
#include "xpano/gui/file_dialog.h"
#include "xpano/utils/imgui_.h"
#include "xpano/version_fmt.h"

namespace xpano::gui {

namespace {
const char* WarningMessage(WarningType warning) {
  switch (warning) {
    case WarningType::kWarnInputConversion:
      return "Only 8-bit stitching pipeline is implemented!\nHigher bit depth "
             "images are converted to 8-bit.";
    case WarningType::kFirstTimeLaunch:
      return "Your friendly panorama stitching app";
    case WarningType::kUserPrefBreakingChange:
      return "The user settings format has "
             "changed, reverting to defaults.";
    case WarningType::kUserPrefCouldntLoad:
      return "Couldn't load user settings, reverting to defaults.";
    case WarningType::kUserPrefResetOnRequest:
      return "User settings were reset to default values,\nyou can keep "
             "experimenting!";
    case WarningType::kNewVersion:
      return "Xpano was updated!";
    case WarningType::kFilePickerUnsupportedExt:
      return "File format is not supported!";
    case WarningType::kFilePickerUnknownError:
      return "File picker error!";
    default:
      return "";
  }
}

const char* Title(WarningType warning) {
  switch (warning) {
    case WarningType::kFirstTimeLaunch:
      return "Welcome to Xpano!";
    case WarningType::kNewVersion:
      return "Version update";
    case WarningType::kUserPrefResetOnRequest:
      return "Info";
    default:
      return "Warning!";
  }
}

bool EnableSnooze(WarningType warning) {
  switch (warning) {
    case WarningType::kWarnInputConversion:
      return true;
    default:
      return false;
  }
}

}  // namespace

void WarningPane::Draw() {
  if (current_warning_.type == WarningType::kNone) {
    if (pending_warnings_.empty()) {
      return;
    }
    Show(pending_warnings_.front());
    pending_warnings_.pop();
  }

  const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

  if (ImGui::BeginPopupModal(Title(current_warning_.type), nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::TextUnformatted(WarningMessage(current_warning_.type));
    DrawExtra(current_warning_);
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("OK", utils::imgui::DpiAwareSize(kWideButtonWidth, 0))) {
      ImGui::CloseCurrentPopup();
      current_warning_ = {WarningType::kNone};
    }
    if (EnableSnooze(current_warning_.type)) {
      ImGui::SameLine();
      bool dont_show_again = dont_show_again_.contains(current_warning_.type);
      if (ImGui::Checkbox("Do not warn next time", &dont_show_again)) {
        if (dont_show_again) {
          dont_show_again_.insert(current_warning_.type);
        } else {
          dont_show_again_.erase(current_warning_.type);
        }
      }
    }
    ImGui::EndPopup();
  }
}

void WarningPane::DrawExtra(const Warning& warning) {
  switch (warning.type) {
    case WarningType::kFirstTimeLaunch: {
      ImGui::Text(
          " - default settings are designed to work out of the box with most "
          "images");
      ImGui::Text(
          " - hover over the little question marks for detailed instructions:");
      ImGui::SameLine();
      utils::imgui::InfoMarker(
          "(?)", "You can try importing a whole directory at once");
      break;
    }
    case WarningType::kNewVersion: {
      ImGui::TextUnformatted(warning.extra_message.c_str());
      if (changelog_) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        utils::imgui::DrawScrollableText(
            "Changelog", changelog_->lines,
            utils::imgui::DpiAwareSize(kAboutBoxWidth, kAboutBoxHeight / 2));
      }
      break;
    }
    case WarningType::kFilePickerUnsupportedExt: {
      ImGui::TextUnformatted(warning.extra_message.c_str());
      break;
    }
    case WarningType::kFilePickerUnknownError: {
      ImGui::BeginChild("FilePickerError",
                        utils::imgui::DpiAwareSize(kAboutBoxWidth, 3));
      ImGui::TextWrapped("%s", warning.extra_message.c_str());
      ImGui::EndChild();
      break;
    }
    default:
      break;
  }
}

void WarningPane::Queue(WarningType warning) {
  pending_warnings_.emplace(warning);
}

void WarningPane::QueueNewVersion(version::Triplet previous_version,
                                  std::optional<utils::Text> changelog) {
  pending_warnings_.emplace(WarningType::kNewVersion,
                            fmt::format(" - from version {} to version {}",
                                        previous_version, version::Current()));
  changelog_ = std::move(changelog);
}

void WarningPane::QueueFilePickerError(const file_dialog::Error& error) {
  switch (error.type) {
    case file_dialog::ErrorType::kUnsupportedExtension: {
      pending_warnings_.emplace(
          WarningType::kFilePickerUnsupportedExt,
          fmt::format("Selected filename: {}\nSupported extensions: {}",
                      error.message, fmt::join(kSupportedExtensions, ", ")));
      break;
    }
    case file_dialog::ErrorType::kUnknownError: {
      pending_warnings_.emplace(WarningType::kFilePickerUnknownError,
                                error.message);
      break;
    }
    default:
      break;
  }
}

void WarningPane::Show(Warning warning) {
  if (!dont_show_again_.contains(warning.type)) {
    ImGui::OpenPopup(Title(warning.type));
    current_warning_ = std::move(warning);
  }
  spdlog::warn(WarningMessage(current_warning_.type));
}

}  // namespace xpano::gui
