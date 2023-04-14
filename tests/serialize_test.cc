#include "xpano/utils/serialize.h"

#include <filesystem>

#include <catch2/catch_test_macros.hpp>

#include "tests/utils.h"
#include "xpano/pipeline/options.h"

struct Bar {
  int first;
  int second;
};

struct BarV2 {
  int first;
  int second;
  int third;
};

struct Foo {
  int first;
  int second;
  Bar bar;
};

struct FooV2 {
  int first;
  int second;
  BarV2 bar;
};

TEST_CASE("Deserialize") {
  auto tmp_path = xpano::tests::TmpPath();
  auto foo = Foo{1, 2, {3, 4}};

  auto error = xpano::utils::serialize::SerializeWithVersion(tmp_path, foo);
  REQUIRE(!error);

  auto [status, foo_recovered] =
      xpano::utils::serialize::DeserializeWithVersion<Foo>(tmp_path);

  REQUIRE(status == xpano::utils::serialize::DeserializeStatus::kSuccess);
  CHECK(foo_recovered.first == 1);
  CHECK(foo_recovered.second == 2);
  CHECK(foo_recovered.bar.first == 3);
  CHECK(foo_recovered.bar.second == 4);
  REQUIRE(std::filesystem::remove(tmp_path));
}

TEST_CASE("Deserialize no such file") {
  const std::filesystem::path path = "no_such_file";

  auto [status, foo_recovered] =
      xpano::utils::serialize::DeserializeWithVersion<Foo>(path);

  REQUIRE(status == xpano::utils::serialize::DeserializeStatus::kNoSuchFile);
}

TEST_CASE("Deserialize breaking change") {
  auto tmp_path = xpano::tests::TmpPath();
  auto foo = Foo{1, 2, {3, 4}};

  auto error = xpano::utils::serialize::SerializeWithVersion(tmp_path, foo);
  REQUIRE(!error);

  auto [status, foo_recovered] =
      xpano::utils::serialize::DeserializeWithVersion<FooV2>(tmp_path);

  REQUIRE(status ==
          xpano::utils::serialize::DeserializeStatus::kBreakingChange);
  REQUIRE(std::filesystem::remove(tmp_path));
}

TEST_CASE("Deserialize pipeline options") {
  auto tmp_path = xpano::tests::TmpPath();
  auto options = xpano::pipeline::Options{};

  options.compression.jpeg_quality = 1;
  options.loading.preview_longer_side = 2;
  options.matching.neighborhood_search_size = 3;
  options.stitch.projection.type = xpano::algorithm::ProjectionType::kPanini;

  auto error = xpano::utils::serialize::SerializeWithVersion(tmp_path, options);
  REQUIRE(!error);

  auto [status, options_recovered] =
      xpano::utils::serialize::DeserializeWithVersion<xpano::pipeline::Options>(
          tmp_path);

  REQUIRE(status == xpano::utils::serialize::DeserializeStatus::kSuccess);
  CHECK(options_recovered.compression.jpeg_quality == 1);
  CHECK(options_recovered.loading.preview_longer_side == 2);
  CHECK(options_recovered.matching.neighborhood_search_size == 3);
  CHECK(options_recovered.stitch.projection.type ==
        xpano::algorithm::ProjectionType::kPanini);

  REQUIRE(std::filesystem::remove(tmp_path));
}
