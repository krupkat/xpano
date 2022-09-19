#include "xpano/algorithm/auto_crop.h"

#include <catch2/catch_test_macros.hpp>
#include <opencv2/core.hpp>

#include "xpano/utils/vec.h"

using xpano::algorithm::crop::FindLargestCrop;
using xpano::algorithm::crop::kMaskValueOn;
using xpano::utils::Point2i;

// NOLINTBEGIN(readability-magic-numbers)

TEST_CASE("Auto crop empty mask") {
  cv::Mat mask(10, 20, CV_8U, cv::Scalar(0));
  auto result = FindLargestCrop(mask);
  CHECK(!result.has_value());
}

TEST_CASE("Auto crop full mask / even size") {
  cv::Mat mask(10, 20, CV_8U, cv::Scalar(kMaskValueOn));
  auto result = FindLargestCrop(mask);
  REQUIRE(result.has_value());
  CHECK(result->start == Point2i{0, 0});
  CHECK(result->end == Point2i{20, 10});
}

TEST_CASE("Auto crop full mask / odd size") {
  cv::Mat mask(10, 21, CV_8U, cv::Scalar(kMaskValueOn));
  auto result = FindLargestCrop(mask);
  REQUIRE(result.has_value());
  CHECK(result->start == Point2i{0, 0});
  CHECK(result->end == Point2i{21, 10});
}

TEST_CASE("Auto crop single column mask") {
  cv::Mat mask(10, 1, CV_8U, cv::Scalar(kMaskValueOn));
  auto result = FindLargestCrop(mask);
  REQUIRE(result.has_value());
  CHECK(result->start == Point2i{0, 0});
  CHECK(result->end == Point2i{1, 10});
}

TEST_CASE("Auto crop two columns mask") {
  cv::Mat mask(10, 2, CV_8U, cv::Scalar(kMaskValueOn));
  auto result = FindLargestCrop(mask);
  REQUIRE(result.has_value());
  CHECK(result->start == Point2i{0, 0});
  CHECK(result->end == Point2i{2, 10});
}

TEST_CASE("Auto crop single row mask") {
  cv::Mat mask(1, 20, CV_8U, cv::Scalar(kMaskValueOn));
  auto result = FindLargestCrop(mask);
  REQUIRE(result.has_value());
  CHECK(result->start == Point2i{0, 0});
  CHECK(result->end == Point2i{20, 1});
}

TEST_CASE("Auto crop two rows mask") {
  cv::Mat mask(2, 20, CV_8U, cv::Scalar(kMaskValueOn));
  auto result = FindLargestCrop(mask);
  REQUIRE(result.has_value());
  CHECK(result->start == Point2i{0, 0});
  CHECK(result->end == Point2i{20, 2});
}

TEST_CASE("Auto crop mask with rows set") {
  cv::Mat mask(10, 20, CV_8U, cv::Scalar(0));

  SECTION("single row") {
    mask.row(5) = kMaskValueOn;
    auto result = FindLargestCrop(mask);
    REQUIRE(result.has_value());
    CHECK(result->start == Point2i{0, 5});
    CHECK(result->end == Point2i{20, 6});
  }

  SECTION("two rows") {
    mask.row(5) = kMaskValueOn;
    mask.row(6) = kMaskValueOn;
    auto result = FindLargestCrop(mask);
    REQUIRE(result.has_value());
    CHECK(result->start == Point2i{0, 5});
    CHECK(result->end == Point2i{20, 7});
  }
}

TEST_CASE("Auto crop mask with empty column") {
  cv::Mat mask(10, 20, CV_8U, cv::Scalar(kMaskValueOn));
  mask.col(5) = 0;
  auto result = FindLargestCrop(mask);
  REQUIRE(result.has_value());
  // Algorithm will stop when encountering empty column 5
  // this is to simplify the implementation
  CHECK(result->start == Point2i{6, 0});
  CHECK(result->end == Point2i{14, 10});
}

TEST_CASE("Auto crop empty matrix") {
  cv::Mat mask;
  auto result = FindLargestCrop(mask);
  REQUIRE(!result.has_value());
}

// Example from https://stackoverflow.com/questions/2478447
/*
      I
    1 1 1 1 0 1
    1 1 0 1 1 0
II->1 1 1 1 1 1
    0 1 1 1 1 1
    1 1 1 1 1 0 <--IV
    1 1 0 1 1 1
            IV
*/
TEST_CASE("Auto crop complex case I") {
  cv::Mat mask(6, 6, CV_8U, cv::Scalar(kMaskValueOn));
  mask.at<unsigned char>(0, 4) = 0;
  mask.at<unsigned char>(1, 2) = 0;
  mask.at<unsigned char>(1, 5) = 0;
  mask.at<unsigned char>(3, 0) = 0;
  mask.at<unsigned char>(4, 5) = 0;
  mask.at<unsigned char>(5, 2) = 0;

  auto result = FindLargestCrop(mask);
  REQUIRE(result.has_value());
  CHECK(result->start == Point2i{1, 2});
  CHECK(result->end == Point2i{5, 5});
}

/*        X
    1 1 1 1 1 1 X
    1 1 1 1 1 1
    1 1 0 1 1 1
    0 1 1 1 1 1
    1 1 1 1 1 1
    1 1 1 1 1 1 X
              X
*/
TEST_CASE("Auto crop complex case II") {
  cv::Mat mask(6, 6, CV_8U, cv::Scalar(kMaskValueOn));
  mask.at<unsigned char>(2, 2) = 0;

  auto result = FindLargestCrop(mask);
  REQUIRE(result.has_value());
  CHECK(result->start == Point2i{3, 0});
  CHECK(result->end == Point2i{6, 6});
}

// NOLINTEND(readability-magic-numbers)
