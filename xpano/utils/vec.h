#pragma once

#include <array>

#include <imgui.h>
#include <opencv2/core.hpp>
#include <SDL.h>

#include <type_traits>

namespace xpano::utils {

struct Vector;
struct Point;
struct Ratio;

template <typename TType, size_t N, typename NameTag = void>
struct Vec {
 public:
  constexpr Vec() = default;
  explicit constexpr Vec(TType value) : data_{} { data_.fill(value); }
  template <typename... Args, typename TEnable = std::enable_if_t<
                                  sizeof...(Args) == N && N != 1, void>>
  // NOLINTNEXTLINE(google-explicit-constructor)
  constexpr Vec(Args... args) : data_{args...} {};

  constexpr TType& operator[](size_t index) { return data_[index]; }
  constexpr const TType& operator[](size_t index) const { return data_[index]; }

  [[nodiscard]] constexpr auto Aspect() const
      -> std::enable_if_t<N == 2, float> {
    return static_cast<float>(data_[0]) / static_cast<float>(data_[1]);
  }

 private:
  std::array<TType, N> data_;
};

using Vec2f = Vec<float, 2, Vector>;
using Vec2i = Vec<int, 2, Vector>;
using Point2f = Vec<float, 2, Point>;
using Point2i = Vec<int, 2, Point>;
using Ratio2f = Vec<float, 2, Ratio>;
using Ratio2i = Vec<int, 2, Ratio>;

namespace internal {

template <typename TType, size_t N, typename Tag, typename TTypeRight,
          std::size_t... Index>
constexpr auto DivideByConstant(const Vec<TType, N, Tag>& lhs, TTypeRight rhs,
                                std::index_sequence<Index...> /*unused*/)
    -> Vec<std::common_type_t<TType, TTypeRight>, N, Tag> {
  return {lhs[Index] / rhs...};
}

template <typename TType, size_t N, typename Tag, typename TTypeRight,
          std::size_t... Index>
constexpr auto MultiplyByConstant(const Vec<TType, N, Tag>& lhs, TTypeRight rhs,
                                  std::index_sequence<Index...> /*unused*/)
    -> Vec<std::common_type_t<TType, TTypeRight>, N, Tag> {
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

template <typename TType, size_t N, typename Tag, std::size_t... Index>
constexpr auto DivideByItself(const Vec<TType, N, Tag>& lhs,
                              const Vec<TType, N, Tag>& rhs,
                              std::index_sequence<Index...> /*unused*/)
    -> Vec<float, N, Ratio> {
  if constexpr (std::is_floating_point_v<TType>) {
    return {lhs[Index] / rhs[Index]...};
  } else {
    return {static_cast<float>(lhs[Index]) / static_cast<float>(rhs[Index])...};
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
                   std::index_sequence<Index...> /*unused*/)
    -> std::enable_if_t<!kBothSameAsV<TagLeft, TagRight, Point> &&
                            !std::is_same_v<TagLeft, Ratio> &&
                            !std::is_same_v<TagRight, Ratio>,
                        Vec<TType, N, AddResultTag<TagLeft, TagRight>>> {
  return {lhs[Index] + rhs[Index]...};
}

template <typename TType, size_t N, typename TagLeft, typename TagRight,
          std::size_t... Index>
constexpr auto Subtract(const Vec<TType, N, TagLeft>& lhs,
                        const Vec<TType, N, TagRight>& rhs,
                        std::index_sequence<Index...> /*unused*/)
    -> std::enable_if_t<
        !(std::is_same_v<TagLeft, Vector> &&
          std::is_same_v<TagRight, Point>)&&(!std::is_same_v<TagLeft, Ratio> &&
                                             !std::is_same_v<TagRight, Ratio>),
        std::conditional_t<std::is_same_v<TagLeft, TagRight>,
                           Vec<TType, N, Vector>, Vec<TType, N, Point>>> {
  return {lhs[Index] - rhs[Index]...};
}

template <typename TType, size_t N, typename NameTag, std::size_t... Index>
constexpr auto ToIntVec(const Vec<TType, N, NameTag>& vec,
                        std::index_sequence<Index...> /*unused*/)
    -> Vec<int, N, NameTag> {
  return {static_cast<int>(vec[Index])...};
}

}  // namespace internal

template <typename TType, size_t N, typename NameTag>
constexpr auto ToIntVec(const Vec<TType, N, NameTag>& vec) {
  return internal::ToIntVec(vec, std::make_index_sequence<N>{});
}

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
constexpr ImVec2 ImVec(const Vec<TType, 2, NameTag>& vec) {
  return {static_cast<float>(vec[0]), static_cast<float>(vec[1])};
}
inline cv::Rect CvRect(Point2i start, Vec2i size) {
  return cv::Rect{start[0], start[1], size[0], size[1]};
}
inline cv::Size CvSize(Vec2i size) { return cv::Size{size[0], size[1]}; }
inline Vec2i ToIntVec(const cv::MatSize& size) {
  return Vec2i{size[1], size[0]};
}
inline SDL_Rect SdlRect(Point2i start, Vec2i size) {
  return SDL_Rect{start[0], start[1], size[0], size[1]};
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
template <typename TType, size_t N, typename Tag, typename TRatioType>
constexpr auto operator*(const Vec<TType, N, Tag>& lhs,
                         const Vec<TRatioType, N, Ratio>& rhs) {
  return internal::MultiplyByRatio(lhs, rhs, std::make_index_sequence<N>{});
}

// Vec / Vec = Ratio
// Point / Point = Ratio
// Ratio / Ratio = Ratio
template <typename TType, size_t N, typename Tag>
constexpr auto operator/(const Vec<TType, N, Tag>& lhs,
                         const Vec<TType, N, Tag>& rhs) {
  return internal::DivideByItself(lhs, rhs, std::make_index_sequence<N>{});
}

}  // namespace xpano::utils
