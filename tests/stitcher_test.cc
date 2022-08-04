#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

#include "algorithm/stitcher_pipeline.h"

using Catch::Matchers::Equals;
using Catch::Matchers::WithinRel;

const std::vector<std::string> kInputs = {
    "data/image00.jpg", "data/image01.jpg", "data/image02.jpg",
    "data/image03.jpg", "data/image04.jpg", "data/image05.jpg",
    "data/image06.jpg", "data/image07.jpg", "data/image08.jpg",
    "data/image09.jpg",
};

TEST_CASE("Stitcher pipeline defaults") {
  xpano::algorithm::StitcherPipeline stitcher;

  auto result = stitcher.RunLoading(kInputs, {}).get();

  CHECK(result.images.size() == 10);
  CHECK(result.matches.size() == 9);
  REQUIRE(result.panos.size() == 2);
  REQUIRE_THAT(result.panos[0].ids, Equals<int>({1, 2, 3, 4, 5}));
  REQUIRE_THAT(result.panos[1].ids, Equals<int>({6, 7, 8}));

  const float eps = 0.01;

  auto pano0 = stitcher.RunStitching(result, 0).get();
  REQUIRE(pano0.has_value());
  CHECK_THAT(pano0->rows, WithinRel(804, eps));
  CHECK_THAT(pano0->cols, WithinRel(2145, eps));

  auto pano1 = stitcher.RunStitching(result, 1).get();
  REQUIRE(pano1.has_value());
  CHECK_THAT(pano1->rows, WithinRel(974, eps));
  CHECK_THAT(pano1->cols, WithinRel(1325, eps));
}
