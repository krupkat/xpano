// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include <vector>

namespace xpano::utils {

class DisjointSet {
 public:
  void Union(int left, int right);
  int Find(int element);

 private:
  void Resize(int element);

  std::vector<int> parent_;
  std::vector<int> rank_;
};

}  // namespace xpano::utils
