// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <cstdint>

#include "xpano/constants.h"

namespace xpano::algorithm {

// Bump kOptionsVersion in xpano/pipeline/options.h when changing the
// definitions

enum class ProjectionType : std::uint8_t {
  kPerspective,
  kCylindrical,
  kSpherical,
  kFisheye,
  kStereographic,
  kCompressedRectilinear,
  kPanini,
  kMercator,
  kTransverseMercator
};

enum class FeatureType : std::uint8_t { kSift, kOrb };

enum class WaveCorrectionType : std::uint8_t {
  kOff,
  kAuto,
  kHorizontal,
  kVertical
};

enum class InpaintingMethod : std::uint8_t {
  kNavierStokes,
  kTelea,
};

enum class BlendingMethod : std::uint8_t { kOpenCV, kMultiblend };

const char* Label(ProjectionType projection_type);
const char* Label(FeatureType feature_type);
const char* Label(WaveCorrectionType wave_correction_type);
const char* Label(InpaintingMethod inpaint_method);
const char* Label(BlendingMethod blending_method);

bool HasAdvancedParameters(ProjectionType projection_type);

const auto kProjectionTypes = std::array{ProjectionType::kPerspective,
                                         ProjectionType::kCylindrical,
                                         ProjectionType::kSpherical,
                                         ProjectionType::kCompressedRectilinear,
                                         ProjectionType::kPanini,
                                         ProjectionType::kMercator,
                                         ProjectionType::kTransverseMercator,
                                         ProjectionType::kFisheye,
                                         ProjectionType::kStereographic};

const auto kFeatureTypes = std::array{FeatureType::kSift, FeatureType::kOrb};

const auto kWaveCorrectionTypes =
    std::array{WaveCorrectionType::kOff, WaveCorrectionType::kAuto,
               WaveCorrectionType::kHorizontal, WaveCorrectionType::kVertical};

const auto kInpaintingMethods =
    std::array{InpaintingMethod::kNavierStokes, InpaintingMethod::kTelea};

const auto kBlendingMethods =
    std::array{BlendingMethod::kOpenCV, BlendingMethod::kMultiblend};

#ifdef XPANO_WITH_MULTIBLEND
const auto kDefaultBlendingMethod = BlendingMethod::kMultiblend;
#else
const auto kDefaultBlendingMethod = BlendingMethod::kOpenCV;
#endif

/*****************************************************************************/

struct ProjectionOptions {
  ProjectionType type = ProjectionType::kSpherical;
  float a_param = kDefaultPaniniA;
  float b_param = kDefaultPaniniB;
};

struct StitchUserOptions {
  ProjectionOptions projection;
  FeatureType feature = FeatureType::kSift;
  WaveCorrectionType wave_correction = WaveCorrectionType::kAuto;
  float match_conf = kDefaultMatchConf;
  int max_pano_mpx = kMaxPanoMpx;
  BlendingMethod blending_method = kDefaultBlendingMethod;
};

struct InpaintingOptions {
  double radius = kDefaultInpaintingRadius;
  InpaintingMethod method = InpaintingMethod::kTelea;
};

}  // namespace xpano::algorithm
