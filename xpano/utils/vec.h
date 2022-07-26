#pragma once

#include <array>

#include <imgui.h>

#include <type_traits>

namespace xpano::utils {

struct Vector;
struct Point;
struct Ratio;

template <typename TType, size_t N, typename NameTag = void>
struct Vec {
  std::array<TType, N> data;
};

using Vec2f = Vec<float, 2, Vector>;
using Vec2i = Vec<int, 2, Vector>;
using Point2f = Vec<float, 2, Point>;
using Point2i = Vec<int, 2, Point>;
using Ratio2f = Vec<float, 2, Ratio>;

namespace internal {

template <typename TType, size_t N, typename Tag, typename TTypeRight,
          std::size_t... Index>
constexpr auto DivideByConstant(const Vec<TType, N, Tag>& lhs, TTypeRight rhs,
                                std::index_sequence<Index...>)
    -> Vec<std::common_type_t<TType, TTypeRight>, N, Tag> {
  return {lhs.data[Index] / rhs...};
}

template <typename TType, size_t N, typename Tag, typename TTypeRight,
          std::size_t... Index>
constexpr auto MultiplyByConstant(const Vec<TType, N, Tag>& lhs, TTypeRight rhs,
                                  std::index_sequence<Index...>)
    -> Vec<std::common_type_t<TType, TTypeRight>, N, Tag> {
  return {lhs.data[Index] * rhs...};
}

template <typename TType, size_t N, typename Tag, std::size_t... Index>
constexpr auto MultiplyByRatio(const Vec<TType, N, Tag>& lhs,
                               const Vec<float, N, Ratio>& rhs,
                               std::index_sequence<Index...>)
    -> Vec<std::common_type_t<TType, float>, N, Tag> {
  return {lhs.data[Index] * rhs.data[Index]...};
}

template <typename TType, size_t N, typename Tag, std::size_t... Index>
constexpr auto DivideByItself(const Vec<TType, N, Tag>& lhs,
                              const Vec<TType, N, Tag>& rhs,
                              std::index_sequence<Index...>)
    -> std::enable_if_t<std::is_same_v<Tag, Vector> ||
                            std::is_same_v<Tag, Point>,
                        Vec<float, N, Ratio>> {
  if constexpr (std::is_floating_point_v<TType>) {
    return {lhs.data[Index] / rhs.data[Index]...};
  } else {
    return {static_cast<float>(lhs.data[Index]) /
            static_cast<float>(rhs.data[Index])...};
  }
}

template <typename TagLeft, typename TagRight, typename TType>
inline constexpr bool kBothSameAsV = (std::is_same_v<TagLeft, TType> &&
                                      std::is_same_v<TagRight, TType>);

template <typename TagLeft, typename TagRight>
using AddResultTag =
    typename std::conditional<kBothSameAsV<TagLeft, TagRight, Vector>, Vector,
                              Point>::type;

template <typename TType, size_t N, typename TagLeft, typename TagRight,
          std::size_t... Index>
constexpr auto Add(const Vec<TType, N, TagLeft>& lhs,
                   const Vec<TType, N, TagRight>& rhs,
                   std::index_sequence<Index...>)
    -> std::enable_if_t<!kBothSameAsV<TagLeft, TagRight, Point> &&
                            !std::is_same_v<TagLeft, Ratio> &&
                            !std::is_same_v<TagRight, Ratio>,
                        Vec<TType, N, AddResultTag<TagLeft, TagRight>>> {
  return {lhs.data[Index] + rhs.data[Index]...};
}

template <typename TType, size_t N, typename TagLeft, typename TagRight,
          std::size_t... Index>
constexpr auto Subtract(const Vec<TType, N, TagLeft>& lhs,
                        const Vec<TType, N, TagRight>& rhs,
                        std::index_sequence<Index...>)
    -> std::enable_if_t<
        !(std::is_same_v<TagLeft, Vector> &&
          std::is_same_v<TagRight, Point>)&&(!std::is_same_v<TagLeft, Ratio> &&
                                             !std::is_same_v<TagRight, Ratio>),
        std::conditional_t<std::is_same_v<TagLeft, TagRight>,
                           Vec<TType, N, Vector>, Vec<TType, N, Point>>> {
  return {lhs.data[Index] - rhs.data[Index]...};
}

}  // namespace internal

constexpr Vec2f ToVec(const ImVec2& imvec) { return Vec2f{imvec.x, imvec.y}; }
constexpr Vec2i ToIntVec(const ImVec2& imvec) {
  return Vec2i{static_cast<int>(imvec.x), static_cast<int>(imvec.y)};
}
constexpr Point2f ToPoint(const ImVec2& imvec) {
  return Point2f{imvec.x, imvec.y};
}
constexpr Point2i ToIntPoint(const ImVec2& imvec) {
  return Point2i{static_cast<int>(imvec.x), static_cast<int>(imvec.y)};
}
template <typename TType, typename NameTag>
constexpr ImVec2 ToImVec(const Vec<TType, 2, NameTag>& vec) {
  return {static_cast<float>(vec.data[0]), static_cast<float>(vec.data[1])};
}

// Vec + Vec = Vec
// Vec + Point = Point
// Point + Vec = Point
template <typename TType, size_t N, typename TagLeft, typename TagRight>
constexpr auto operator+(const Vec<TType, N, TagLeft>& lhs,
                         const Vec<TType, N, TagRight>& rhs) {
  return internal::Add(lhs, rhs, std::make_index_sequence<N>{});
}

// Vec - Vec = Vec
// Point - Vec = Point
// Point - Point = Vec
template <typename TType, size_t N, typename TagLeft, typename TagRight>
constexpr auto operator-(const Vec<TType, N, TagLeft>& lhs,
                         const Vec<TType, N, TagRight>& rhs) {
  return internal::Subtract(lhs, rhs, std::make_index_sequence<N>{});
}

// T / constant = T<common_type<T, constant>>
template <typename TType, size_t N, typename Tag, typename TTypeRight>
constexpr auto operator/(const Vec<TType, N, Tag>& lhs, TTypeRight rhs) {
  return internal::DivideByConstant(lhs, rhs, std::make_index_sequence<N>{});
}

// T * constant = T<common_type<T, constant>>
template <typename TType, size_t N, typename Tag, typename TTypeRight>
constexpr auto operator*(const Vec<TType, N, Tag>& lhs, TTypeRight rhs) {
  return internal::MultiplyByConstant(lhs, rhs, std::make_index_sequence<N>{});
}

// T * Ratio = T<common_type<T, ratio>
template <typename TType, size_t N, typename Tag>
constexpr auto operator*(const Vec<TType, N, Tag>& lhs,
                         const Vec<float, N, Ratio>& rhs) {
  return internal::MultiplyByRatio(lhs, rhs, std::make_index_sequence<N>{});
}

// Vec / Vec = Ratio
// Point / Point = Ratio
template <typename TType, size_t N, typename TagLeft, typename TagRight>
constexpr auto operator/(const Vec<TType, N, TagLeft>& lhs,
                         const Vec<TType, N, TagRight>& rhs) {
  return internal::DivideByItself(lhs, rhs, std::make_index_sequence<N>{});
}

}  // namespace xpano::utils
