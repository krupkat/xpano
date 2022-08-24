#include "utils/disjoint_set.h"

#include <numeric>
#include <utility>

namespace xpano::utils {

DisjointSet::DisjointSet(int size) : parent_(size), rank_(size, 0) {
  std::iota(parent_.begin(), parent_.end(), 0);
};

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
  while (element != parent_[element]) {
    parent_[element] = parent_[parent_[element]];
    element = parent_[element];
  }
  return element;
}

}  // namespace xpano::utils
