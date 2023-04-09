#pragma once

#include <filesystem>
#include <random>
#include <string>

namespace xpano::tests {

std::string TmpPath() {
  std::string alphanum_chars(
      "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
  std::mt19937 generator(std::random_device{}());
  std::shuffle(alphanum_chars.begin(), alphanum_chars.end(), generator);
  const int filename_length = 32;
  return (std::filesystem::temp_directory_path() /
          alphanum_chars.substr(0, filename_length))
      .string();
}

}  // namespace xpano::tests
