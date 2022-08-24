#pragma once

#include <array>
#include <chrono>
#include <string>

namespace xpano {

constexpr int kNumFeatures = 3000;
constexpr int kPreviewSize = 256;
constexpr int kMaxTexSize = 16384;
constexpr int kLoupeSize = 4096;
constexpr int kMatchThreshold = 70;

constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 800;

constexpr float kZoomFactor = 2.0f;
constexpr int kZoomLevels = 10;

constexpr int kResizingDelayFrames = 30;
constexpr int kScrollingSpeed = 150;

const std::array<std::string, 5> kSupportedExtensions = {"jpg", "jpeg", "tiff",
                                                         "png", "bmp"};

const std::string kLogFilename = "logs/xpano.log";
constexpr int kMaxLogSize = 5 * 1024 * 1024;
constexpr int kMaxLogFiles = 5;

const char* const kCheckMark = reinterpret_cast<const char*>(u8"✓");

constexpr auto kTaskCancellationTimeout = std::chrono::milliseconds(500);

constexpr int kDefaultJpegQuality = 95;
constexpr int kMaxJpegQuality = 100;
constexpr int kDefaultPngCompression = 6;
constexpr int kMaxPngCompression = 9;

constexpr int kAboutBoxWidth = 70;
constexpr int kAboutBoxHeight = 30;
constexpr int kSidebarWidth = 23;

const std::string kOrgName = "krupkat";
const std::string kAppName = "Xpano";

const std::string kLicensePath = "licenses";
const std::string kFontPath = "assets/NotoSans-Regular.ttf";
const std::string kSymbolsFontPath = "assets/NotoSansSymbols2-Regular.ttf";
const std::string kIconPath = "assets/icon.png";

constexpr int kDefaultNeighborhoodSearchSize = 2;

}  // namespace xpano
