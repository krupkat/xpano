#pragma once

#include <mutex>
#include <string>
#include <vector>

namespace xpano::logger {

class Logger {
 public:
  Logger() = default;

  void Append(const char *message);

  const std::vector<std::string> &Log();

 private:
  void Concatenate();

  std::vector<std::string> log_;
  std::vector<std::string> log_tmp_;

  std::mutex mut_;
};

class LoggerGui {
 public:
  void Draw();
  void RedirectSDLOutput();
  void RedirectOpenCVOutput();

 private:
  Logger logger_;
};

}  // namespace xpano::logger
