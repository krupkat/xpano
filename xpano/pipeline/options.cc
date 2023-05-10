// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xpano/pipeline/options.h"

namespace xpano::pipeline {

const char *Label(ChromaSubsampling subsampling) {
  switch (subsampling) {
    case ChromaSubsampling::k444:
      return "Off";
    case ChromaSubsampling::k422:
      return "Half";
    case ChromaSubsampling::k420:
      return "Quarter";
    default:
      return "Unknown";
  }
}

const char *Label(MatchingType type) {
  switch (type) {
    case MatchingType::kNone:
      return "Off";
    case MatchingType::kSinglePano:
      return "Single pano";
    case MatchingType::kAuto:
      return "Auto";
    default:
      return "Unknown";
  }
}

}  // namespace xpano::pipeline
