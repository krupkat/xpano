#pragma once

#include <stdio.h>

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
  bool attached_console = false;
  FILE* attached_stdout = nullptr;
};

}  // namespace xpano::cli::windows
