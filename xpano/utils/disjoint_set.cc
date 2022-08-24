#include "utils/disjoint_set.h"

#include <numeric>

namespace xpano::utils {

DisjointSet::DisjointSet(int size) : parent(size), rank(size, 0) {
  std::iota(parent.begin(), parent.end(), 0);
};

void DisjointSet::Union(int left, int right) {
  left = Find(left);
  right = Find(right);

  if (left == right) {
    return;
  }

  if (rank[left] < rank[right]) {
    std::swap(left, right);
  }

  parent[right] = left;

  if (rank[left] == rank[right]) {
    ++rank[left];
  }
}

// Path compression
int DisjointSet::Find(int element) {
  if (element != parent[element]) {
    parent[element] = Find(parent[element]);
    return parent[element];
  }
  return element;
}

}  // namespace xpano::utils
