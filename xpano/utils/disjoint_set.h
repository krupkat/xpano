#include <vector>

namespace xpano::utils {

class DisjointSet {
 public:
  DisjointSet(int size);
  void Union(int left, int right);
  int Find(int element);

 private:
  std::vector<int> parent;
  std::vector<int> rank;
};

}  // namespace xpano::utils
