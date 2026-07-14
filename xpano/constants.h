// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <chrono>
#include <string_view>

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

constexpr std::array<std::string_view, 6> kSupportedExtensions = {
    "jpg", "jpeg", "tiff", "tif", "png", "bmp"};

constexpr std::array<std::string_view, 4> kMetadataSupportedExtensions = {
    "jpg", "jpeg", "tiff", "tif"};

constexpr std::string_view kLogFilename = "logs/xpano.log";
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
constexpr int kSidebarWidth = 35;
constexpr int kWideButtonWidth = 12;

constexpr std::string_view kOrgName = "krupkat";
constexpr std::string_view kAppName = "Xpano";

constexpr std::string_view kLicensePath = "licenses";
constexpr std::string_view kFontPath = "assets/NotoSans-Regular.ttf";
constexpr std::string_view kSymbolsFontPath =
    "assets/NotoSansSymbols2-Regular.ttf";
constexpr std::string_view kIconPath = "assets/icon.png";

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

constexpr std::string_view kAppConfigFilename = "app_config.alpaca";
constexpr std::string_view kUserConfigFilename = "user_config.alpaca";
constexpr std::string_view kChangelogFilename = "CHANGELOG.md";

constexpr int kCropEdgeTolerance = 10;
constexpr int kAutoCropSamplingDistance = 512;

constexpr double kDefaultInpaintingRadius = 3.0;
constexpr double kMaxInpaintingRadius = 15.0;
constexpr double kInpaintingRadiusStep = 1.0;
constexpr float kMegapixel = 1'000'000;

constexpr std::string_view kDefaultPanoSuffix = "_pano";
constexpr int kMaxImageSizeForCLI = 8192;

constexpr int kExifDefaultOrientation = 1;

constexpr int kCancelAnimationFrameDuration = 128;

constexpr std::string_view kGithubIssuesLink =
    "https://github.com/krupkat/xpano/issues";
constexpr std::string_view kAuthorEmail = "tomas@krupkat.cz";

constexpr int kMaxPanoMpx = 100;

}  // namespace xpano
