#include <vector>

namespace xpano::utils {

class DisjointSet {
 public:
  explicit DisjointSet(int size);
  void Union(int left, int right);
  int Find(int element);

 private:
  std::vector<int> parent_;
  std::vector<int> rank_;
};

}  // namespace xpano::utils
