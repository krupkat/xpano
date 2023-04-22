#pragma once

#include <cstdio>

namespace xpano::cli::windows {

class Attach {
 public:
  Attach();
  ~Attach();

  Attach(const Attach&) = delete;
  Attach& operator=(const Attach&) = delete;
  Attach(Attach&&) = delete;
  Attach& operator=(Attach&&) = delete;

 private:
  bool attached_console_ = false;
  FILE* attached_stdout_ = nullptr;
};

}  // namespace xpano::cli::windows
