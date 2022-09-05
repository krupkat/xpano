#include "xpano/utils/disjoint_set.h"

#include <catch2/catch_test_macros.hpp>

using xpano::utils::DisjointSet;

TEST_CASE("DisjointSet constructor") {
  auto set = DisjointSet(3);
  CHECK(set.Find(0) == 0);
  CHECK(set.Find(1) == 1);
  CHECK(set.Find(2) == 2);
}

TEST_CASE("DisjointSet Union/Find") {
  auto set = DisjointSet(3);

  set.Union(0, 1);
  CHECK(set.Find(0) == set.Find(1));
  CHECK(set.Find(2) == 2);

  set.Union(1, 2);
  CHECK(set.Find(0) == set.Find(1));
  CHECK(set.Find(1) == set.Find(2));
}
