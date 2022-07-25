#include "log/logger.h"

#include <algorithm>
#include <iterator>
#include <mutex>
#include <string>
#include <vector>

#include <imgui.h>
#include <SDL.h>

namespace xpano::logger {

namespace {
void CustomLog(void *userdata, int /*category*/, SDL_LogPriority /*priority*/,
               const char *message) {
  auto *logger = static_cast<Logger *>(userdata);
  logger->Append(message);
}
}  // namespace

void Logger::Append(const char *message) {
  std::lock_guard<std::mutex> lock(mut_);
  log_tmp_.emplace_back(std::string(message));
}

const std::vector<std::string> &Logger::Log() {
  Concatenate();
  return log_;
}

void Logger::Concatenate() {
  std::lock_guard<std::mutex> lock(mut_);
  if (!log_tmp_.empty()) {
    std::copy(log_tmp_.begin(), log_tmp_.end(), std::back_inserter(log_));
    log_tmp_.resize(0);
  }
}

void LoggerGui::Draw() {
  ImGui::Begin("Logger");
  const auto &log = logger_.Log();
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
  for (const auto &line : log) {
    ImGui::TextUnformatted(line.c_str());
  }
  ImGui::PopStyleVar();
  if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
    ImGui::SetScrollHereY(1.0f);
  }
  ImGui::End();
}

void LoggerGui::RedirectSDLOutput() {
  SDL_LogSetOutputFunction(CustomLog, &logger_);
}

void LoggerGui::RedirectOpenCVOutput() {}

}  // namespace xpano::logger
