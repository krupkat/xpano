#pragma once

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
}  // namespace xpano
