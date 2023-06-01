// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-FileCopyrightText: 2022 Naachiket Pant
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include <spdlog/details/log_msg.h>
#include <spdlog/sinks/base_sink.h>

namespace xpano::logger {

class BufferSinkMt final : public spdlog::sinks::base_sink<std::mutex> {
 public:
  std::vector<std::string> LastFormatted();

 protected:
  void sink_it_(const spdlog::details::log_msg &msg) override;
  void flush_() override;

 private:
  std::vector<std::string> messages_;
};

class Logger {
 public:
  Logger();
  ~Logger();

  Logger(const Logger &) = delete;
  Logger &operator=(const Logger &) = delete;
  Logger(Logger &&) = delete;
  Logger &operator=(Logger &&) = delete;

  const std::vector<std::string> &Log();
  void RedirectSpdlogToGui(std::optional<std::filesystem::path> app_data_path);

  std::optional<std::string> GetLogDirPath();

 private:
  void Concatenate();

  std::vector<std::string> log_;
  std::shared_ptr<BufferSinkMt> sink_;

  std::optional<std::string> log_dir_path_;
};

void RedirectSDLOutput();

void RedirectSpdlogToCout();

}  // namespace xpano::logger
