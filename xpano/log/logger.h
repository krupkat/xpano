#pragma once

#include <mutex>
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
  const std::vector<std::string> &Log();
  void RedirectSpdlogOutput();

 private:
  void Concatenate();

  std::vector<std::string> log_;
  std::shared_ptr<BufferSinkMt> sink_;
};

class LoggerGui {
 public:
  void Draw();
  void RedirectSDLOutput();
  void RedirectOpenCVOutput();
  void RedirectSpdlogOutput();

 private:
  Logger logger_;
};

}  // namespace xpano::logger
