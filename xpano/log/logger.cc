#include "xpano/log/logger.h"

#include <algorithm>
#include <iterator>
#include <mutex>
#include <string>
#include <vector>

#include <SDL.h>
#include <spdlog/common.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

#include "xpano/constants.h"

namespace xpano::logger {

namespace {
void CustomLog(void * /*userdata*/, int /*category*/, SDL_LogPriority priority,
               const char *message) {
  switch (priority) {
    case SDL_LOG_PRIORITY_VERBOSE:
      spdlog::trace(message);
      break;
    case SDL_LOG_PRIORITY_DEBUG:
      spdlog::debug(message);
      break;
    case SDL_LOG_PRIORITY_INFO:
      spdlog::info(message);
      break;
    case SDL_LOG_PRIORITY_WARN:
      spdlog::warn(message);
      break;
    case SDL_LOG_PRIORITY_ERROR:
      spdlog::error(message);
      break;
    case SDL_LOG_PRIORITY_CRITICAL:
      spdlog::critical(message);
      break;
    default:
      spdlog::info(message);
      break;
  }
}
}  // namespace

std::vector<std::string> BufferSinkMt::LastFormatted() {
  std::lock_guard<std::mutex> lock(base_sink<std::mutex>::mutex_);
  std::vector<std::string> new_messages;
  std::swap(new_messages, messages_);
  return new_messages;
}

void BufferSinkMt::sink_it_(const spdlog::details::log_msg &msg) {
  spdlog::memory_buf_t formatted;
  base_sink<std::mutex>::formatter_->format(msg, formatted);
  messages_.push_back(fmt::to_string(formatted));
}

void BufferSinkMt::flush_() {}

Logger::Logger() : sink_(std::make_shared<BufferSinkMt>()) {}

void Logger::RedirectSpdlogToGui(
    std::optional<std::filesystem::path> app_data_path) {
  std::vector<spdlog::sink_ptr> sinks;
  sink_->set_pattern("[%l] %v");
  sinks.push_back(sink_);

  if (app_data_path) {
    auto log_path = *app_data_path / kLogFilename;
    log_dir_path_ = app_data_path->string();

    sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        log_path.string(), kMaxLogSize, kMaxLogFiles, true));
  }

  auto logger =
      std::make_shared<spdlog::logger>("XPano", sinks.begin(), sinks.end());
  logger->flush_on(spdlog::level::err);
  spdlog::set_default_logger(logger);
}

const std::vector<std::string> &Logger::Log() {
  Concatenate();
  return log_;
}

void Logger::Concatenate() {
  auto new_messages = sink_->LastFormatted();
  std::copy(new_messages.begin(), new_messages.end(), std::back_inserter(log_));
}

std::optional<std::string> Logger::GetLogDirPath() { return log_dir_path_; }

void RedirectSDLOutput() { SDL_LogSetOutputFunction(CustomLog, nullptr); }

void RedirectSpdlogToCout() {
  auto logger = spdlog::stdout_logger_mt("console");
  logger->flush_on(spdlog::level::info);
  logger->set_pattern("%l: %v");
  spdlog::set_default_logger(logger);
};

}  // namespace xpano::logger
