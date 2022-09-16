#pragma once

#include <concepts>

#include "xpano/utils/vec.h"

namespace xpano::utils {

template <typename TTypeLeft, typename TTypeRight>
requires std::same_as<typename TTypeLeft::NameTag,
                      typename TTypeRight::NameTag> &&
    std::same_as<typename TTypeLeft::ValueType, typename TTypeRight::ValueType>
struct RectStartEnd {
  TTypeLeft start;
  TTypeRight end;
};

template <typename TTypeLeft, typename TTypeRight>
requires std::same_as<typename TTypeLeft::NameTag, utils::Point> &&
    std::same_as<typename TTypeRight::NameTag, utils::Vector> &&
    std::same_as<typename TTypeLeft::ValueType, typename TTypeRight::ValueType>
struct RectStartSize {
  TTypeLeft start;
  TTypeRight size;
};

using RectRRf = RectStartEnd<Ratio2f, Ratio2f>;
using RectPVf = RectStartSize<Point2f, Vec2f>;

template <typename TTypeLeft, typename TTypeRight>
auto Rect(TTypeLeft left, TTypeRight right) {
  if constexpr (std::same_as<typename TTypeLeft::NameTag, utils::Point> &&
                std::same_as<typename TTypeRight::NameTag, utils::Vector>) {
    return RectStartSize<TTypeLeft, TTypeRight>{left, right};
  } else {
    return RectStartEnd<TTypeLeft, TTypeRight>{left, right};
  }
}

}  // namespace xpano::utils
