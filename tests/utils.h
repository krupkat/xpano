#pragma once

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <random>
#include <string>

namespace xpano::tests {

class Args {
 public:
  template <typename... TArgs>
  Args(TArgs&&... args) : argc(sizeof...(TArgs)), argv(new char*[argc]) {
    int i = 0;
    ((argv[i] = new char[sizeof(args) + 1], std::strcpy(argv[i], args), ++i),
     ...);
  }

  Args(const Args&) = delete;
  Args& operator=(const Args&) = delete;
  Args(Args&&) = delete;
  Args& operator=(Args&&) = delete;

  ~Args() {
    for (int i = 0; i < argc; ++i) {
      delete[] argv[i];
    }
    delete[] argv;
  }

  int GetArgc() const { return argc; }
  char** GetArgv() const { return argv; }

 private:
  int argc;
  char** argv;
};

inline std::filesystem::path TmpPath() {
  std::string alphanum_chars(
      "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
  std::mt19937 generator(std::random_device{}());
  std::shuffle(alphanum_chars.begin(), alphanum_chars.end(), generator);
  const int filename_length = 32;
  return (std::filesystem::temp_directory_path() /
          alphanum_chars.substr(0, filename_length));
}

}  // namespace xpano::tests
