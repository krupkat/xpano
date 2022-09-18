#include "xpano/utils/rect.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "xpano/utils/vec.h"

using Catch::Approx;
using xpano::utils::Aspect;
using xpano::utils::Point2f;
using xpano::utils::Rect;
using xpano::utils::Vec2f;

template <typename TRectType>
concept HasStart = requires(TRectType rect) {
  rect.start;
};

template <typename TRectType>
concept HasSize = requires(TRectType rect) {
  rect.size;
};

template <typename TRectType>
concept HasEnd = requires(TRectType rect) {
  rect.end;
};

TEST_CASE("Rect from Point and Vec") {
  auto rect = Rect(Point2f{1.0f, 2.0f}, Vec2f{1.0f, 2.0f});

  CHECK(HasStart<decltype(rect)>);
  CHECK(HasSize<decltype(rect)>);
  CHECK(!HasEnd<decltype(rect)>);
}

TEST_CASE("Rect from two Points") {
  auto rect = Rect(Point2f{1.0f, 2.0f}, Point2f{2.0f, 4.0f});

  CHECK(HasStart<decltype(rect)>);
  CHECK(!HasSize<decltype(rect)>);
  CHECK(HasEnd<decltype(rect)>);
}

TEST_CASE("Rect Aspect") {
  auto rect_pv = Rect(Point2f{1.0f, 2.0f}, Vec2f{1.0f, 2.0f});
  auto rect_pp = Rect(Point2f{1.0f, 2.0f}, Point2f{2.0f, 4.0f});

  CHECK(Aspect(rect_pv) == Approx(0.5f));
  CHECK(Aspect(rect_pp) == Approx(0.5f));
}

TEST_CASE("Rect Area") {
  auto rect_pv = Rect(Point2f{1.0f, 2.0f}, Vec2f{1.0f, 2.0f});
  auto rect_pp = Rect(Point2f{1.0f, 2.0f}, Point2f{2.0f, 4.0f});

  CHECK(Area(rect_pv) == Approx(2.0f));
  CHECK(Area(rect_pp) == Approx(2.0f));
}
