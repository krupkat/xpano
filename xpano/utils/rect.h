// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <concepts>

#include "xpano/utils/vec.h"

namespace xpano::utils {

struct StartEnd;
struct StartSize;

template <typename TTypeLeft, typename TTypeRight>
  requires std::same_as<typename TTypeLeft::NameTag,
                        typename TTypeRight::NameTag> &&
           std::same_as<typename TTypeLeft::ValueType,
                        typename TTypeRight::ValueType>
struct RectStartEnd {
  using RectType = StartEnd;
  TTypeLeft start;
  TTypeRight end;
};

template <typename TTypeLeft, typename TTypeRight>
  requires std::same_as<typename TTypeLeft::NameTag, utils::Point> &&
           std::same_as<typename TTypeRight::NameTag, utils::Vector> &&
           std::same_as<typename TTypeLeft::ValueType,
                        typename TTypeRight::ValueType>
struct RectStartSize {
  using RectType = StartSize;
  TTypeLeft start;
  TTypeRight size;
};

using RectRRf = RectStartEnd<Ratio2f, Ratio2f>;
using RectPVf = RectStartSize<Point2f, Vec2f>;
using RectPPf = RectStartEnd<Point2f, Point2f>;
using RectPPi = RectStartEnd<Point2i, Point2i>;

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

template <typename TRectType>
auto Area(const TRectType& rect) {
  if constexpr (std::same_as<typename TRectType::RectType, StartEnd>) {
    auto size = rect.end - rect.start;
    return MultiplyElements(size);
  }
  if constexpr (std::same_as<typename TRectType::RectType, StartSize>) {
    return MultiplyElements(rect.size);
  }
}

constexpr auto DefaultCropRect() { return Rect(Ratio2f{0.0f}, Ratio2f{1.0f}); }

inline auto CropRectPP(const utils::RectPVf& image,
                       const utils::RectRRf& crop_rect) {
  return utils::Rect(image.start + image.size * crop_rect.start,
                     image.start + image.size * crop_rect.end);
}

}  // namespace xpano::utils
