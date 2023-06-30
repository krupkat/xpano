// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xpano/algorithm/progress.h"

namespace xpano::algorithm {

void ProgressMonitor::Reset(ProgressType type, int num_tasks) {
  type_ = type;
  done_ = 0;
  num_tasks_ = num_tasks;
}

void ProgressMonitor::SetTaskType(ProgressType type) { type_ = type; }

void ProgressMonitor::SetNumTasks(int num_tasks) { num_tasks_ = num_tasks; }

ProgressReport ProgressMonitor::Report() const {
  return {type_, done_, num_tasks_};
}

void ProgressMonitor::NotifyTaskDone() { done_++; }

void ProgressMonitor::Cancel() { cancel_ = true; }

bool ProgressMonitor::IsCancelled() const { return cancel_; }

}  // namespace xpano::algorithm
