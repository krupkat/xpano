// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <chrono>
#include <string>

namespace xpano {

constexpr int kNumFeatures = 3000;
constexpr int kThumbnailSize = 256;
constexpr int kMaxTexSize = 16384;
constexpr int kLoupeSize = 4096;
constexpr int kMinMatchThreshold = 4;
constexpr int kDefaultMatchThreshold = 70;
constexpr int kMaxMatchThreshold = 250;

constexpr float kMinShiftInPano = 0.0f;
constexpr float kDefaultShiftInPano = 0.1f;
constexpr float kMaxShiftInPano = 1.0f;

constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 800;
constexpr int kMinWindowSize = 200;

constexpr float kZoomFactor = 1.4f;
constexpr int kZoomLevels = 11;
constexpr float kZoomSpeed = 0.1f;

constexpr int kResizingDelayFrames = 30;
constexpr int kScrollingStep = 200;
constexpr int kScrollingStepPerFrame = 25;

const std::array<std::string, 6> kSupportedExtensions = {"jpg", "jpeg", "tiff",
                                                         "tif", "png",  "bmp"};

const std::array<std::string, 4> kMetadataSupportedExtensions = {"jpg", "jpeg",
                                                                 "tiff", "tif"};

const std::string kLogFilename = "logs/xpano.log";
constexpr int kMaxLogSize = 5 * 1024 * 1024;
constexpr int kMaxLogFiles = 5;

const char* const kCheckMark = reinterpret_cast<const char*>(u8"✓");
const char* const kCommandSymbol = reinterpret_cast<const char*>(u8"⌘");

constexpr auto kTaskCancellationTimeout = std::chrono::milliseconds(500);
constexpr auto kCancellationTimeout = std::chrono::milliseconds(500);

constexpr int kDefaultJpegQuality = 95;
constexpr int kMaxJpegQuality = 100;
constexpr int kDefaultPngCompression = 6;
constexpr int kMaxPngCompression = 9;

constexpr int kAboutBoxWidth = 70;
constexpr int kAboutBoxHeight = 30;
constexpr int kSidebarWidth = 27;
constexpr int kWideButtonWidth = 12;

const std::string kOrgName = "krupkat";
const std::string kAppName = "Xpano";

const std::string kLicensePath = "licenses";
const std::string kFontPath = "assets/NotoSans-Regular.ttf";
const std::string kSymbolsFontPath = "assets/NotoSansSymbols2-Regular.ttf";
const std::string kIconPath = "assets/icon.png";

constexpr int kDefaultNeighborhoodSearchSize = 2;
constexpr int kMaxNeighborhoodSearchSize = 10;

constexpr int kDefaultPreviewLongerSide = 1024;
constexpr int kMinPreviewLongerSide = 512;
constexpr int kMaxPreviewLongerSide = 2048;
constexpr int kStepPreviewLongerSide = 256;

constexpr float kDefaultPaniniA = 2.0f;
constexpr float kDefaultPaniniB = 1.0f;
constexpr float kDefaultMatchConf = 0.25f;
constexpr float kMinMatchConf = 0.1f;
constexpr float kMaxMatchConf = 0.4f;

const std::string kAppConfigFilename = "app_config.alpaca";
const std::string kUserConfigFilename = "user_config.alpaca";
const std::string kChangelogFilename = "CHANGELOG.md";

constexpr int kCropEdgeTolerance = 10;
constexpr int kAutoCropSamplingDistance = 512;

constexpr double kDefaultInpaintingRadius = 3.0;
constexpr double kMaxInpaintingRadius = 15.0;
constexpr double kInpaintingRadiusStep = 1.0;
constexpr float kMegapixel = 1'000'000;

const std::string kDefaultPanoSuffix = "_pano";
constexpr int kMaxImageSizeForCLI = 8192;

constexpr int kExifDefaultOrientation = 1;

constexpr int kCancelAnimationFrameDuration = 128;

const std::string kGithubIssuesLink = "https://github.com/krupkat/xpano/issues";
const std::string kAuthorEmail = "tomas@krupkat.cz";

const int kMaxPanoSize = 16384;

}  // namespace xpano
