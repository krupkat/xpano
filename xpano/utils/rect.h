#pragma once

#include <concepts>

#include "xpano/utils/vec.h"

namespace xpano::utils {

struct StartEnd;
struct StartSize;

template <typename TTypeLeft, typename TTypeRight>
requires std::same_as<typename TTypeLeft::NameTag,
                      typename TTypeRight::NameTag> &&
    std::same_as<typename TTypeLeft::ValueType, typename TTypeRight::ValueType>
struct RectStartEnd {
  using RectType = StartEnd;
  TTypeLeft start;
  TTypeRight end;
};

template <typename TTypeLeft, typename TTypeRight>
requires std::same_as<typename TTypeLeft::NameTag, utils::Point> &&
    std::same_as<typename TTypeRight::NameTag, utils::Vector> &&
    std::same_as<typename TTypeLeft::ValueType, typename TTypeRight::ValueType>
struct RectStartSize {
  using RectType = StartSize;
  TTypeLeft start;
  TTypeRight size;
};

using RectRRf = RectStartEnd<Ratio2f, Ratio2f>;
using RectPVf = RectStartSize<Point2f, Vec2f>;

template <typename TTypeLeft, typename TTypeRight>
constexpr auto Rect(TTypeLeft left, TTypeRight right) {
  if constexpr (std::same_as<typename TTypeLeft::NameTag, utils::Point> &&
                std::same_as<typename TTypeRight::NameTag, utils::Vector>) {
    return RectStartSize<TTypeLeft, TTypeRight>{left, right};
  } else {
    return RectStartEnd<TTypeLeft, TTypeRight>{left, right};
  }
}

template <typename TRectType>
auto Aspect(const TRectType& rect) {
  if constexpr (std::same_as<typename TRectType::RectType, StartEnd>) {
    return (rect.end - rect.start).Aspect();
  }
  if constexpr (std::same_as<typename TRectType::RectType, StartSize>) {
    return rect.size.Aspect();
  }
}

}  // namespace xpano::utils
