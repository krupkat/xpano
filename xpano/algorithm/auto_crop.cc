// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xpano/algorithm/auto_crop.h"

#include <algorithm>
#include <optional>
#include <vector>

#include <opencv2/core.hpp>

#include "xpano/constants.h"
#include "xpano/utils/rect.h"
#include "xpano/utils/vec.h"

namespace xpano::algorithm::crop {

namespace {
struct Line {
  int start;
  int end;
};

bool IsLineValid(const Line& line) { return line.start < line.end; }

int Length(const Line& line) { return line.end - line.start; }

bool IsSet(unsigned char value) { return value == kMaskValueOn; }

std::optional<Line> FindLongestLineInColumn(cv::Mat column) {
  std::optional<Line> current_line;
  std::optional<Line> longest_line;

  if (IsSet(column.at<unsigned char>(0, 0))) {
    current_line = Line{0, 1};
  }

  for (int i = 1; i < column.rows; i++) {
    auto prev = IsSet(column.at<unsigned char>(i - 1, 0));
    auto current = IsSet(column.at<unsigned char>(i, 0));

    if (!prev && current) {
      current_line = Line{i, i + 1};
    }

    if (prev && current) {
      current_line->end = i + 1;
    }

    if (prev && !current) {
      if (!longest_line || Length(*current_line) > Length(*longest_line)) {
        longest_line = current_line;
      }
      current_line.reset();
    }
  }

  if (IsSet(column.at<unsigned char>(column.rows - 1, 0))) {
    if (!longest_line || Length(*current_line) > Length(*longest_line)) {
      longest_line = current_line;
    }
  }

  return longest_line;
}

std::optional<utils::RectPPi> FindLargestCrop(const std::vector<Line>& lines,
                                              const Line& invalid_line,
                                              int seed) {
  if (!IsLineValid(lines[seed])) {
    return {};
  }
  auto current_rect =
      utils::RectPPi{{seed, lines[seed].start}, {seed + 1, lines[seed].end}};
  auto largest_rect = current_rect;

  if (lines.size() == 1) {
    return largest_rect;
  }

  int left = seed - 1;
  int right = seed + static_cast<int>(lines.size() % 2);
  auto left_line = lines[left];
  auto right_line = lines[right];

  while (IsLineValid(left_line) || IsLineValid(right_line)) {
    auto left_rect = utils::RectPPi{
        {left, std::max(left_line.start, current_rect.start[1])},
        {current_rect.end[0], std::min(left_line.end, current_rect.end[1])}};

    auto right_rect = utils::RectPPi{
        {current_rect.start[0],
         std::max(right_line.start, current_rect.start[1])},
        {right + 1, std::min(right_line.end, current_rect.end[1])}};

    if (utils::Area(left_rect) > utils::Area(right_rect)) {
      current_rect = left_rect;
      left_line = (left == 0) ? invalid_line : lines[--left];
    } else {
      current_rect = right_rect;
      right_line = (right == lines.size() - 1) ? invalid_line : lines[++right];
    }

    if (utils::Area(current_rect) >= utils::Area(largest_rect)) {
      largest_rect = current_rect;
    }
  }

  return largest_rect;
}

}  // namespace

// Approximage solution only.
// Full solution would be https://stackoverflow.com/questions/2478447
// This algorithm starts in multiple sampled locations and expands
// the rectangles in the direction with the larger area.
std::optional<utils::RectPPi> FindLargestCrop(const cv::Mat& mask) {
  if (mask.empty()) {
    return {};
  }
  const Line invalid_line = {mask.rows, 0};
  std::vector<Line> lines(mask.cols);
  for (int i = 0; i < mask.cols; i++) {
    auto longest_line = FindLongestLineInColumn(mask.col(i));
    lines[i] = longest_line.value_or(invalid_line);
  }

  std::optional<utils::RectPPi> largest_rect;

  const int num_samples = 1 + mask.cols / kAutoCropSamplingDistance;
  for (int i = 0; i < num_samples; i++) {
    const int start = (i + 1) * mask.cols / (num_samples + 1);
    auto current_rect = FindLargestCrop(lines, invalid_line, start);

    if (current_rect && (!largest_rect || utils::Area(*current_rect) >=
                                              utils::Area(*largest_rect))) {
      largest_rect = current_rect;
    }
  }

  return largest_rect;
}

}  // namespace xpano::algorithm::crop
