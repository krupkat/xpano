// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>

#include "xpano/constants.h"

namespace xpano::algorithm {

// Bump kOptionsVersion in xpano/pipeline/options.h when changing the
// definitions

enum class ProjectionType {
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

enum class FeatureType { kSift, kOrb };

enum class WaveCorrectionType { kOff, kAuto, kHorizontal, kVertical };

enum class InpaintingMethod {
  kNavierStokes,
  kTelea,
};

enum class BlendingMethod { kOpenCV, kMultiblend };

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
  BlendingMethod blending_method = kDefaultBlendingMethod;
};

struct InpaintingOptions {
  double radius = kDefaultInpaintingRadius;
  InpaintingMethod method = InpaintingMethod::kTelea;
};

}  // namespace xpano::algorithm
