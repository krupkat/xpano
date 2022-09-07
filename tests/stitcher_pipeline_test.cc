#include "xpano/algorithm/stitcher_pipeline.h"

#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

using Catch::Matchers::Equals;
using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;

const std::vector<std::string> kInputs = {
    "data/image00.jpg", "data/image01.jpg", "data/image02.jpg",
    "data/image03.jpg", "data/image04.jpg", "data/image05.jpg",
    "data/image06.jpg", "data/image07.jpg", "data/image08.jpg",
    "data/image09.jpg",
};

TEST_CASE("Stitcher pipeline defaults") {
  xpano::algorithm::StitcherPipeline stitcher;

  auto result = stitcher.RunLoading(kInputs, {}, {}).get();
  auto progress = stitcher.LoadingProgress();
  CHECK(progress.tasks_done == progress.num_tasks);

  CHECK(result.images.size() == 10);
  CHECK(result.matches.size() == 17);
  REQUIRE(result.panos.size() == 2);
  REQUIRE_THAT(result.panos[0].ids, Equals<int>({1, 2, 3, 4, 5}));
  REQUIRE_THAT(result.panos[1].ids, Equals<int>({6, 7, 8}));

  const float eps = 0.01;

  auto pano0 = stitcher.RunStitching(result, {.pano_id = 0}).get().pano;
  REQUIRE(pano0.has_value());
  CHECK_THAT(pano0->rows, WithinRel(804, eps));
  CHECK_THAT(pano0->cols, WithinRel(2145, eps));

  auto pano1 = stitcher.RunStitching(result, {.pano_id = 1}).get().pano;
  REQUIRE(pano1.has_value());
  CHECK_THAT(pano1->rows, WithinRel(974, eps));
  CHECK_THAT(pano1->cols, WithinRel(1325, eps));
}

const std::vector<std::string> kShuffledInputs = {
    "data/image01.jpg",  // Pano 1
    "data/image06.jpg",  // 2
    "data/image02.jpg",  // Pano 1
    "data/image07.jpg",  // 2
    "data/image03.jpg",  // Pano 1
    "data/image00.jpg",
    "data/image08.jpg",  // 2
    "data/image04.jpg",  // Pano 1
    "data/image09.jpg",
    "data/image05.jpg",  // Pano 1
};

TEST_CASE("Stitcher pipeline custom matching neighborhood") {
  xpano::algorithm::StitcherPipeline stitcher;

  auto result =
      stitcher.RunLoading(kShuffledInputs, {}, {.neighborhood_search_size = 3})
          .get();
  auto progress = stitcher.LoadingProgress();
  CHECK(progress.tasks_done == progress.num_tasks);

  CHECK(result.images.size() == 10);
  CHECK(result.matches.size() == 24);
  REQUIRE(result.panos.size() == 2);
  REQUIRE_THAT(result.panos[0].ids, Equals<int>({0, 2, 4, 7, 9}));
  REQUIRE_THAT(result.panos[1].ids, Equals<int>({1, 3, 6}));
}

// NOLINTBEGIN(readability-magic-numbers)

TEST_CASE("Stitcher pipeline larger neighborhood size") {
  xpano::algorithm::StitcherPipeline stitcher;

  auto result = stitcher
                    .RunLoading({"data/image01.jpg", "data/image02.jpg",
                                 "data/image03.jpg"},
                                {}, {.neighborhood_search_size = 10})
                    .get();

  auto progress = stitcher.LoadingProgress();
  CHECK(progress.tasks_done == progress.num_tasks);

  REQUIRE(result.images.size() == 3);
  REQUIRE(result.matches.size() == 3);  // [0 + 1], [0 + 2], [1 + 2]
}

// NOLINTEND(readability-magic-numbers)

TEST_CASE("Stitcher pipeline single image") {
  xpano::algorithm::StitcherPipeline stitcher;

  auto result = stitcher.RunLoading({"data/image01.jpg"}, {}, {}).get();
  auto progress = stitcher.LoadingProgress();
  CHECK(progress.tasks_done == progress.num_tasks);

  REQUIRE(result.images.size() == 1);
  REQUIRE(result.matches.empty());
}

TEST_CASE("Stitcher pipeline no images") {
  xpano::algorithm::StitcherPipeline stitcher;

  auto result = stitcher.RunLoading({}, {}, {}).get();
  auto progress = stitcher.LoadingProgress();
  CHECK(progress.tasks_done == progress.num_tasks);

  REQUIRE(result.images.empty());
  REQUIRE(result.matches.empty());
}

TEST_CASE("Stitcher pipeline loading options") {
  xpano::algorithm::StitcherPipeline stitcher;

  const int preview_size = 512;
  const int allowed_margin = 1.0;

  auto result = stitcher
                    .RunLoading({"data/image05.jpg", "data/image06.jpg"},
                                {.preview_longer_side = preview_size}, {})
                    .get();
  auto progress = stitcher.LoadingProgress();
  CHECK(progress.tasks_done == progress.num_tasks);

  REQUIRE(result.images.size() == 2);

  auto landscape_full = result.images[0].GetFullRes();
  auto landscape_full_size = landscape_full.size();
  auto landscape_preview = result.images[0].GetPreview();
  auto landscape_preview_size = landscape_preview.size();

  CHECK(landscape_preview_size.width == preview_size);
  CHECK_THAT(landscape_preview_size.height,
             WithinAbs(preview_size / landscape_full_size.aspectRatio(),
                       allowed_margin));

  auto portrait_full = result.images[1].GetFullRes();
  auto portrait_full_size = portrait_full.size();
  auto portrait_preview = result.images[1].GetPreview();
  auto portrait_preview_size = portrait_preview.size();

  CHECK(portrait_preview_size.height == preview_size);
  CHECK_THAT(portrait_preview_size.width,
             WithinAbs(preview_size * portrait_full_size.aspectRatio(),
                       allowed_margin));
}
