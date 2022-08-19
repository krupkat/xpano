#pragma once

#include <string>
#include <vector>

namespace xpano::utils {

struct Text {
  std::string name;
  std::vector<std::string> lines;
  bool operator== (const Text& operand) {
    return ((name == operand.name) && (lines == operand.lines));
  }
};

std::vector<Text> LoadTexts(const std::string& executable_path,
                            const std::string& rel_path);

}  // namespace xpano::utils
