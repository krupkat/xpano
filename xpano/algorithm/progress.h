// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <atomic>
#include <cstdint>

namespace xpano::algorithm {

enum class ProgressType : std::uint8_t {
  kNone,
  kLoadingImages,
  kStitchingPano,
  kAutoCrop,
  kDetectingKeypoints,
  kMatchingImages,
  kExport,
  kInpainting,
  kStitchFindFeatures,
  kStitchMatchFeatures,
  kStitchEstimateHomography,
  kStitchBundleAdjustment,
  kStitchComputeRoi,
  kStitchSeamsPrepare,
  kStitchSeamsFind,
  kStitchCompose,
  kStitchBlend,
  kCancelling
};

struct ProgressReport {
  ProgressType type = ProgressType::kNone;
  int tasks_done = 0;
  int num_tasks = 0;
};

class ProgressMonitor {
 public:
  void Reset(ProgressType type, int num_tasks);
  void SetNumTasks(int num_tasks);
  void SetTaskType(ProgressType type);
  [[nodiscard]] ProgressReport Report() const;
  void NotifyTaskDone();
  void Cancel();
  [[nodiscard]] bool IsCancelled() const;

 private:
  std::atomic<ProgressType> type_{ProgressType::kNone};
  std::atomic<int> done_ = 0;
  std::atomic<int> num_tasks_ = 0;
  std::atomic<bool> cancel_ = false;
};

}  // namespace xpano::algorithm
