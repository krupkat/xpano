#include "xpano/utils/vec.h"

#include <concepts>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;
using xpano::utils::Point2f;
using xpano::utils::Point2i;
using xpano::utils::Ratio2f;
using xpano::utils::Ratio2i;
using xpano::utils::Vec2f;
using xpano::utils::Vec2i;

// NOLINTBEGIN(readability-magic-numbers)

TEMPLATE_TEST_CASE("Vec constructor + access", "[Vec]", Vec2f, Point2f,
                   Ratio2f) {
  TestType vec{1.0f, 2.0f};
  REQUIRE(vec[0] == 1.0f);
  REQUIRE(vec[1] == 2.0f);
}

TEMPLATE_TEST_CASE("Vec constructor + access", "[Vec]", Vec2i, Point2i,
                   Ratio2i) {
  TestType vec{1, 2};
  REQUIRE(vec[0] == 1);
  REQUIRE(vec[1] == 2);
}

TEST_CASE("Vec constructor") {
  REQUIRE(std::constructible_from<Vec2f, float, float>);
  REQUIRE(std::constructible_from<Vec2f, float>);

  REQUIRE(!std::constructible_from<Vec2f, int>);
  REQUIRE(!std::constructible_from<Vec2f, int, int>);
  REQUIRE(!std::constructible_from<Vec2f, float, int>);
  REQUIRE(!std::constructible_from<Vec2f, int, float>);
  REQUIRE(!std::constructible_from<Vec2f, float, float, float>);
}

TEST_CASE("Vec aspect (float)") {
  Vec2f vec{1.0f, 2.0f};
  REQUIRE(vec.Aspect() == Approx(0.5f));
}

TEST_CASE("Vec aspect (int)") {
  Vec2i vec{1, 2};
  REQUIRE(vec.Aspect() == Approx(0.5f));
}

TEST_CASE("ToIntVec") {
  Vec2f vec{1.1f, 2.3f};
  auto result = ToIntVec(vec);
  REQUIRE(result[0] == 1);
  REQUIRE(result[1] == 2);
}

TEST_CASE("Add") {
  Vec2f vec1 = {1.0f, 2.0f};
  Vec2f vec2 = {0.2f, 1.5f};
  Point2f point1 = {3.0f, 4.0f};

  SECTION("Vec + Vec") {
    auto result = vec1 + vec2;
    REQUIRE(result[0] == Approx(1.2f));
    REQUIRE(result[1] == Approx(3.5f));
  }

  SECTION("Vec + Point") {
    auto result = vec1 + point1;
    REQUIRE(result[0] == Approx(4.0f));
    REQUIRE(result[1] == Approx(6.0f));
  }

  SECTION("Point + Vec") {
    auto result = point1 + vec1;
    REQUIRE(result[0] == Approx(4.0f));
    REQUIRE(result[1] == Approx(6.0f));
  }
}

template <typename TLeft, typename TRight, typename TResult = void>
concept Addable = requires(TLeft lhs, TRight rhs, TResult result) {
  { lhs + rhs } -> std::same_as<TResult>;
};

TEST_CASE("Add type checks") {
  REQUIRE(Addable<Vec2f, Vec2f, Vec2f>);
  REQUIRE(Addable<Vec2f, Point2f, Point2f>);
  REQUIRE(Addable<Point2f, Vec2f, Point2f>);
  REQUIRE(!Addable<Point2f, Point2f>);
}

TEST_CASE("Subtract") {
  Vec2f vec1 = {1.0f, 2.0f};
  Vec2f vec2 = {0.2f, 1.5f};
  Point2f point1 = {3.0f, 4.0f};
  Point2f point2 = {2.0f, 3.0f};

  SECTION("Vec - Vec") {
    auto result = vec1 - vec2;
    REQUIRE(result[0] == Approx(0.8f));
    REQUIRE(result[1] == Approx(0.5f));
  }

  SECTION("Point - Vec") {
    auto result = point1 - vec1;
    REQUIRE(result[0] == Approx(2.0f));
    REQUIRE(result[1] == Approx(2.0f));
  }

  SECTION("Point - Point") {
    auto result = point1 - point2;
    REQUIRE(result[0] == Approx(1.0f));
    REQUIRE(result[1] == Approx(1.0f));
  }
}

template <typename TLeft, typename TRight, typename TResult = void>
concept Subtractable = requires(TLeft lhs, TRight rhs, TResult result) {
  { lhs - rhs } -> std::same_as<TResult>;
};

TEST_CASE("Subtract type checks") {
  REQUIRE(Subtractable<Vec2f, Vec2f, Vec2f>);
  REQUIRE(!Subtractable<Vec2f, Point2f>);
  REQUIRE(Subtractable<Point2f, Vec2f, Point2f>);
  REQUIRE(Subtractable<Point2f, Point2f, Vec2f>);

  REQUIRE(Subtractable<Ratio2f, Ratio2f, Ratio2f>);
  REQUIRE(!Subtractable<Vec2f, Ratio2f>);
  REQUIRE(!Subtractable<Point2f, Ratio2f>);
  REQUIRE(!Subtractable<Ratio2f, Vec2f>);
  REQUIRE(!Subtractable<Ratio2f, Point2f>);
}

TEST_CASE("Divide by constant") {
  Vec2f vec1 = {5.0f, 10.0f};
  Vec2i vec2 = {5, 10};

  SECTION("Vec2f / float") {
    auto result = vec1 / 2.0f;
    REQUIRE(result[0] == Approx(2.5f));
    REQUIRE(result[1] == Approx(5.0f));
  }

  SECTION("Vec2f / int") {
    auto result = vec1 / 2;
    REQUIRE(result[0] == Approx(2.5f));
    REQUIRE(result[1] == Approx(5.0f));
  }

  SECTION("Vec2i / float") {
    auto result = vec2 / 2.0f;
    REQUIRE(result[0] == Approx(2.5f));
    REQUIRE(result[1] == Approx(5.0f));
  }

  SECTION("Vec2i / int") {
    auto result = vec2 / 2;
    REQUIRE(result[0] == 2);
    REQUIRE(result[1] == 5);
  }
}

TEST_CASE("Divide by Vec") {
  Vec2f vec1 = {5.0f, 10.0f};
  Vec2f vec2 = {2.0f, 5.0f};

  Vec2i vec1i = {5, 10};
  Vec2i vec2i = {2, 5};

  SECTION("Vec2f / Vec2f") {
    auto result = vec1 / vec2;
    REQUIRE(result[0] == Approx(2.5f));
    REQUIRE(result[1] == Approx(2.0f));
  }

  SECTION("Vec2i / Vec2i") {
    auto result = vec1i / vec2i;
    REQUIRE(result[0] == Approx(2.5f));
    REQUIRE(result[1] == Approx(2.0f));
  }
}

template <typename TLeft, typename TRight, typename TResult = void>
concept Divisible = requires(TLeft lhs, TRight rhs, TResult result) {
  { lhs / rhs } -> std::same_as<TResult>;
};

TEST_CASE("Divide type checks") {
  SECTION("Divide by constant") {
    REQUIRE(Divisible<Vec2f, float, Vec2f>);
    REQUIRE(Divisible<Vec2f, int, Vec2f>);
    REQUIRE(Divisible<Vec2i, float, Vec2f>);
    REQUIRE(Divisible<Vec2i, int, Vec2i>);
  }

  SECTION("Divide by same type") {
    REQUIRE(Divisible<Vec2f, Vec2f, Ratio2f>);
    REQUIRE(Divisible<Vec2i, Vec2i, Ratio2f>);
  }

  SECTION("Non divisible") {
    REQUIRE(!Divisible<Vec2i, Vec2f>);
    REQUIRE(!Divisible<Vec2f, Vec2i>);
    REQUIRE(!Divisible<Vec2f, Point2f>);
    REQUIRE(!Divisible<Point2f, Vec2f>);
    REQUIRE(!Divisible<Vec2f, Ratio2f>);
    REQUIRE(!Divisible<Ratio2f, Vec2f>);
  }
}

TEST_CASE("Multiply by constant") {
  Vec2f vec1 = {1.0f, 2.0f};
  Vec2i vec2 = {1, 2};

  SECTION("Vec2f * float") {
    auto result = vec1 * 1.5f;
    REQUIRE(result[0] == Approx(1.5f));
    REQUIRE(result[1] == Approx(3.0f));
  }

  SECTION("Vec2f * int") {
    auto result = vec1 * 2;
    REQUIRE(result[0] == Approx(2.0f));
    REQUIRE(result[1] == Approx(4.0f));
  }

  SECTION("Vec2i * float") {
    auto result = vec2 * 1.5f;
    REQUIRE(result[0] == Approx(1.5f));
    REQUIRE(result[1] == Approx(3.0f));
  }

  SECTION("Vec2i * int") {
    auto result = vec2 * 2;
    REQUIRE(result[0] == 2);
    REQUIRE(result[1] == 4);
  }
}

TEST_CASE("Multiply by Ratio") {
  Vec2f vec1 = {5.0f, 10.0f};
  Vec2i vec2 = {5, 10};

  Ratio2f ratio1 = {2.5f, 5.0f};
  Ratio2i ratio2 = {2, 5};

  SECTION("Vec2f * Ratio2f") {
    auto result = vec1 * ratio1;
    REQUIRE(result[0] == Approx(12.5f));
    REQUIRE(result[1] == Approx(50.0f));
  }

  SECTION("Vec2f * Ratio2i") {
    auto result = vec1 * ratio2;
    REQUIRE(result[0] == Approx(10.0f));
    REQUIRE(result[1] == Approx(50.0f));
  }

  SECTION("Vec2i * Ratio2f") {
    auto result = vec2 * ratio1;
    REQUIRE(result[0] == Approx(12.5f));
    REQUIRE(result[1] == Approx(50.0f));
  }

  SECTION("Vec2i * Ratio2i") {
    auto result = vec1 * ratio2;
    REQUIRE(result[0] == 10);
    REQUIRE(result[1] == 50);
  }
}

template <typename TLeft, typename TRight, typename TResult = void>
concept Multiplicable = requires(TLeft lhs, TRight rhs, TResult result) {
  { lhs* rhs } -> std::same_as<TResult>;
};

TEST_CASE("Multiply type checks") {
  SECTION("Multiply by constant") {
    REQUIRE(Multiplicable<Vec2f, float, Vec2f>);
    REQUIRE(Multiplicable<Vec2f, int, Vec2f>);
    REQUIRE(Multiplicable<Vec2i, float, Vec2f>);
    REQUIRE(Multiplicable<Vec2i, int, Vec2i>);
  }

  SECTION("Multiply by Ratio") {
    REQUIRE(Multiplicable<Vec2f, Ratio2f, Vec2f>);
    REQUIRE(Multiplicable<Vec2f, Ratio2i, Vec2f>);
    REQUIRE(Multiplicable<Vec2i, Ratio2f, Vec2f>);
    REQUIRE(Multiplicable<Vec2i, Ratio2i, Vec2i>);
  }

  SECTION("Non multiplicable") {
    REQUIRE(!Multiplicable<Vec2f, Vec2f>);
    REQUIRE(!Multiplicable<Point2f, Point2f>);
  }
}

// NOLINTEND(readability-magic-numbers)
