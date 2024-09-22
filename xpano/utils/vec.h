// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <ostream>
#include <type_traits>
#include <utility>

namespace xpano::utils {

struct Vector;
struct Point;
struct Ratio;

template <typename TType, size_t N, typename TNameTag = void>
struct Vec {
 public:
  using ValueType = TType;
  using NameTag = TNameTag;

  constexpr Vec() = default;

  template <typename TFillType>
    requires std::same_as<TType, TFillType>
  explicit constexpr Vec(TFillType value) {
    data_.fill(value);
  }

  template <typename... Args>
    requires(std::same_as<TType, Args> && ...) && (sizeof...(Args) == N)
  // NOLINTNEXTLINE(google-explicit-constructor)
  constexpr Vec(Args... args) : data_{args...} {}

  constexpr TType& operator[](size_t index) { return data_[index]; }
  constexpr const TType& operator[](size_t index) const { return data_[index]; }

  [[nodiscard]] constexpr float Aspect() const
    requires(N == 2)
  {
    return static_cast<float>(data_[0]) / static_cast<float>(data_[1]);
  }

 private:
  std::array<TType, N> data_{};
};

using Vec2f = Vec<float, 2, Vector>;
using Vec2i = Vec<int, 2, Vector>;
using Point2f = Vec<float, 2, Point>;
using Point2i = Vec<int, 2, Point>;
using Ratio2f = Vec<float, 2, Ratio>;
using Ratio2i = Vec<int, 2, Ratio>;

namespace internal {

template <typename TType, size_t N, typename Tag, typename TRight,
          std::size_t... Index>
constexpr auto DivideByConstant(const Vec<TType, N, Tag>& lhs, TRight rhs,
                                std::index_sequence<Index...> /*unused*/)
    -> Vec<std::common_type_t<TType, TRight>, N, Tag> {
  return {lhs[Index] / rhs...};
}

template <typename TType, size_t N, typename Tag, std::size_t... Index>
constexpr auto DivideByItself(
    const Vec<TType, N, Tag>& lhs, const Vec<TType, N, Tag>& rhs,
    std::index_sequence<Index...> /*unused*/) -> Vec<float, N, Ratio> {
  if constexpr (std::is_floating_point_v<TType>) {
    return {lhs[Index] / rhs[Index]...};
  } else {
    return {static_cast<float>(lhs[Index]) / static_cast<float>(rhs[Index])...};
  }
}

template <typename TType, size_t N, typename Tag, typename TRight,
          std::size_t... Index>
constexpr auto MultiplyByConstant(const Vec<TType, N, Tag>& lhs, TRight rhs,
                                  std::index_sequence<Index...> /*unused*/)
    -> Vec<std::common_type_t<TType, TRight>, N, Tag> {
  return {lhs[Index] * rhs...};
}

template <typename TType, size_t N, typename Tag, typename TRatioType,
          std::size_t... Index>
constexpr auto MultiplyByRatio(const Vec<TType, N, Tag>& lhs,
                               const Vec<TRatioType, N, Ratio>& rhs,
                               std::index_sequence<Index...> /*unused*/)
    -> Vec<std::common_type_t<TType, TRatioType>, N, Tag> {
  return {lhs[Index] * rhs[Index]...};
}

template <typename TagLeft, typename TagRight>
concept Addable =
    !(std::same_as<TagLeft, Point> && std::same_as<TagRight, Point>) &&
    !std::same_as<TagLeft, Ratio> && !std::same_as<TagRight, Ratio>;

template <typename TagLeft, typename TagRight>
using AddResultTag = std::conditional_t<std::is_same_v<TagLeft, Vector> &&
                                            std::is_same_v<TagRight, Vector>,
                                        Vector, Point>;

template <typename TType, size_t N, typename TagLeft, typename TagRight,
          std::size_t... Index>
constexpr auto Add(const Vec<TType, N, TagLeft>& lhs,
                   const Vec<TType, N, TagRight>& rhs,
                   std::index_sequence<Index...> /*unused*/)
    -> Vec<TType, N, AddResultTag<TagLeft, TagRight>> {
  return {lhs[Index] + rhs[Index]...};
}

template <typename TagLeft, typename TagRight>
concept Subtractable =
    (std::same_as<TagLeft, Vector> && std::same_as<TagRight, Vector>) ||
    (std::same_as<TagLeft, Point> && std::same_as<TagRight, Vector>) ||
    (std::same_as<TagLeft, Point> && std::same_as<TagRight, Point>) ||
    (std::same_as<TagLeft, Ratio> && std::same_as<TagRight, Ratio>);

template <typename TagLeft, typename TagRight>
using SubtractResultTag = std::conditional_t<
    std::is_same_v<TagLeft, Ratio> && std::is_same_v<TagRight, Ratio>, Ratio,
    std::conditional_t<std::is_same_v<TagLeft, TagRight>, Vector, Point>>;

template <typename TType, size_t N, typename TagLeft, typename TagRight,
          std::size_t... Index>
constexpr auto Subtract(const Vec<TType, N, TagLeft>& lhs,
                        const Vec<TType, N, TagRight>& rhs,
                        std::index_sequence<Index...> /*unused*/)
    -> Vec<TType, N, SubtractResultTag<TagLeft, TagRight>> {
  return {lhs[Index] - rhs[Index]...};
}

template <typename TType, size_t N, typename NameTag, std::size_t... Index>
constexpr auto ToIntVec(const Vec<TType, N, NameTag>& vec,
                        std::index_sequence<Index...> /*unused*/)
    -> Vec<int, N, NameTag> {
  return {static_cast<int>(vec[Index])...};
}

template <typename TType, size_t N, typename NameTag, std::size_t... Index>
constexpr auto MultiplyElements(const Vec<TType, N, NameTag>& vec,
                                std::index_sequence<Index...> /*unused*/) {
  return (vec[Index] * ...);
}

template <typename TType, size_t N, typename Tag, std::size_t... Index>
constexpr auto Equals(const Vec<TType, N, Tag>& lhs,
                      const Vec<TType, N, Tag>& rhs,
                      std::index_sequence<Index...> /*unused*/) -> bool {
  return ((lhs[Index] == rhs[Index]) && ...);
}

template <typename TType, size_t N, typename Tag, std::size_t... Index>
std::ostream& Print(std::ostream& ostream, const Vec<TType, N, Tag>& vec,
                    std::index_sequence<Index...> /*unused*/) {
  ((ostream << vec[Index] << ' '), ...);
  return ostream;
}

}  // namespace internal

template <typename TType, size_t N, typename NameTag>
constexpr auto ToIntVec(const Vec<TType, N, NameTag>& vec) {
  return internal::ToIntVec(vec, std::make_index_sequence<N>{});
}

// Vec + Vec = Vec
// Vec + Point = Point
// Point + Vec = Point
template <typename TType, size_t N, typename TagLeft, typename TagRight>
  requires internal::Addable<TagLeft, TagRight>
constexpr auto operator+(const Vec<TType, N, TagLeft>& lhs,
                         const Vec<TType, N, TagRight>& rhs) {
  return internal::Add(lhs, rhs, std::make_index_sequence<N>{});
}

// Vec - Vec = Vec
// Point - Vec = Point
// Point - Point = Vec
// Ratio - Ratio = Ratio
template <typename TType, size_t N, typename TagLeft, typename TagRight>
  requires internal::Subtractable<TagLeft, TagRight>
constexpr auto operator-(const Vec<TType, N, TagLeft>& lhs,
                         const Vec<TType, N, TagRight>& rhs) {
  return internal::Subtract(lhs, rhs, std::make_index_sequence<N>{});
}

// T / constant = T<common_type<T, constant>>
template <typename TType, size_t N, typename Tag, typename TRight>
  requires std::common_with<TType, TRight>
constexpr auto operator/(const Vec<TType, N, Tag>& lhs, TRight rhs) {
  return internal::DivideByConstant(lhs, rhs, std::make_index_sequence<N>{});
}

// Vec / Vec = Ratio
// Point / Point = Ratio
// Ratio / Ratio = Ratio
template <typename TType, size_t N, typename Tag>
constexpr auto operator/(const Vec<TType, N, Tag>& lhs,
                         const Vec<TType, N, Tag>& rhs) {
  return internal::DivideByItself(lhs, rhs, std::make_index_sequence<N>{});
}

// T * constant = T<common_type<T, constant>>
template <typename TType, size_t N, typename Tag, typename TRight>
  requires std::common_with<TType, TRight>
constexpr auto operator*(const Vec<TType, N, Tag>& lhs, TRight rhs) {
  return internal::MultiplyByConstant(lhs, rhs, std::make_index_sequence<N>{});
}

// T * Ratio = T<common_type<T, ratio>
template <typename TType, size_t N, typename Tag, typename TRatioType>
constexpr auto operator*(const Vec<TType, N, Tag>& lhs,
                         const Vec<TRatioType, N, Ratio>& rhs) {
  return internal::MultiplyByRatio(lhs, rhs, std::make_index_sequence<N>{});
}

template <typename TType, size_t N, typename Tag>
constexpr auto MultiplyElements(const Vec<TType, N, Tag>& vec) {
  return internal::MultiplyElements(vec, std::make_index_sequence<N>{});
}

template <typename TType, size_t N, typename Tag>
constexpr auto operator==(const Vec<TType, N, Tag>& lhs,
                          const Vec<TType, N, Tag>& rhs) {
  return internal::Equals(lhs, rhs, std::make_index_sequence<N>{});
}

template <typename TType, size_t N, typename Tag>
std::ostream& operator<<(std::ostream& ostream, const Vec<TType, N, Tag>& vec) {
  return internal::Print(ostream, vec, std::make_index_sequence<N>{});
}

}  // namespace xpano::utils
