// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xpano/utils/disjoint_set.h"

#include <numeric>
#include <utility>
#include <vector>

namespace xpano::utils {

void DisjointSet::Union(int left, int right) {
  left = Find(left);
  right = Find(right);

  if (left == right) {
    return;
  }

  if (rank_[left] < rank_[right]) {
    std::swap(left, right);
  }

  parent_[right] = left;

  if (rank_[left] == rank_[right]) {
    ++rank_[left];
  }
}

// Path halving algorithm from
// https://en.wikipedia.org/wiki/Disjoint-set_data_structure
int DisjointSet::Find(int element) {
  Resize(element);
  while (element != parent_[element]) {
    parent_[element] = parent_[parent_[element]];
    element = parent_[element];
  }
  return element;
}

void DisjointSet::Resize(int element) {
  if (auto old_size = std::ssize(parent_); element >= old_size) {
    parent_.resize(element + 1);
    std::iota(parent_.begin() + old_size, parent_.end(),
              static_cast<int>(old_size));
    rank_.resize(element + 1, 0);
  }
}

}  // namespace xpano::utils
