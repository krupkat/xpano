// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xpano/algorithm/options.h"

namespace xpano::algorithm {

// NOLINTBEGIN(bugprone-branch-clone): doesn't work with [[fallthrough]]

bool HasAdvancedParameters(ProjectionType projection_type) {
  switch (projection_type) {
    case ProjectionType::kCompressedRectilinear:
      [[fallthrough]];
    case ProjectionType::kPanini:
      return true;
    default:
      return false;
  }
}

// NOLINTEND(bugprone-branch-clone)

const char* Label(ProjectionType projection_type) {
  switch (projection_type) {
    case ProjectionType::kPerspective:
      return "Perspective";
    case ProjectionType::kCylindrical:
      return "Cylindrical";
    case ProjectionType::kSpherical:
      return "Spherical";
    case ProjectionType::kFisheye:
      return "*Fisheye";
    case ProjectionType::kStereographic:
      return "*Stereographic";
    case ProjectionType::kCompressedRectilinear:
      return "CompressedRectilinear";
    case ProjectionType::kPanini:
      return "Panini";
    case ProjectionType::kMercator:
      return "Mercator";
    case ProjectionType::kTransverseMercator:
      return "TransverseMercator";
    default:
      return "Unknown";
  }
}

const char* Label(FeatureType feature_type) {
  switch (feature_type) {
    case FeatureType::kSift:
      return "SIFT";
    case FeatureType::kOrb:
      return "ORB";
    default:
      return "Unknown";
  }
}

const char* Label(WaveCorrectionType wave_correction_type) {
  switch (wave_correction_type) {
    case WaveCorrectionType::kOff:
      return "Off";
    case WaveCorrectionType::kAuto:
      return "Auto";
    case WaveCorrectionType::kHorizontal:
      return "Horizontal";
    case WaveCorrectionType::kVertical:
      return "Vertical";
    default:
      return "Unknown";
  }
}

const char* Label(InpaintingMethod inpaint_method) {
  switch (inpaint_method) {
    case InpaintingMethod::kNavierStokes:
      return "NavierStokes";
    case InpaintingMethod::kTelea:
      return "Telea";
    default:
      return "Unknown";
  }
}

const char* Label(BlendingMethod blending_method) {
  switch (blending_method) {
    case BlendingMethod::kOpenCV:
      return "OpenCV";
    case BlendingMethod::kMultiblend:
      return "Multiblend";
    case BlendingMethod::kMultiblendAlpha:
      return "MultiblendAlpha";
    default:
      return "Unknown";
  }
}

}  // namespace xpano::algorithm
