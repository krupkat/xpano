// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>

#include "xpano/algorithm/options.h"
#include "xpano/constants.h"
#include "xpano/utils/exiv2.h"

namespace xpano::pipeline {

// Used for serialization, bump when changing the options structs.
//  - Major changes can be auto detected by alpaca reflection, but e.g.
//    modifying the enums cannot, so bump the version number in this case.
//  - Will result in reloading the default values when loading the config.
constexpr int kOptionsVersion = 2;

enum class ChromaSubsampling {
  k444,
  k422,
  k420,
};

const char *Label(ChromaSubsampling subsampling);

const auto kSubsamplingModes = std::array{
    ChromaSubsampling::k444, ChromaSubsampling::k422, ChromaSubsampling::k420};

enum class MatchingType { kNone, kSinglePano, kAuto };

const char *Label(MatchingType type);

const auto kMatchingTypes = std::array{
    MatchingType::kAuto, MatchingType::kSinglePano, MatchingType::kNone};

/*****************************************************************************/

struct MetadataOptions {
  bool copy_from_first_image = utils::exiv2::Enabled();
};

struct CompressionOptions {
  int jpeg_quality = kDefaultJpegQuality;
  bool jpeg_progressive = false;
  bool jpeg_optimize = false;
  ChromaSubsampling jpeg_subsampling = ChromaSubsampling::k422;
  int png_compression = kDefaultPngCompression;
};

struct LoadingOptions {
  int preview_longer_side = kDefaultPreviewLongerSide;
};

using InpaintingOptions = algorithm::InpaintingOptions;

struct MatchingOptions {
  MatchingType type = MatchingType::kAuto;
  int neighborhood_search_size = kDefaultNeighborhoodSearchSize;
  int match_threshold = kDefaultMatchThreshold;
  float match_conf = kDefaultMatchConf;
};

using StitchAlgorithmOptions = algorithm::StitchOptions;

struct Options {
  MetadataOptions metadata;
  CompressionOptions compression;
  LoadingOptions loading;
  InpaintingOptions inpaint;
  MatchingOptions matching;
  StitchAlgorithmOptions stitch;
};

}  // namespace xpano::pipeline
