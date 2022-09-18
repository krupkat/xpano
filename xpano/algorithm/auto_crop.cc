#include "xpano/algorithm/auto_crop.h"

#include <optional>

#include <opencv2/core.hpp>

#include "xpano/utils/rect.h"
#include "xpano/utils/vec.h"

namespace xpano::algorithm::crop {

namespace {
struct Line {
  int start;
  int end;
};

int Length(const Line& line) { return line.end - line.start; }

bool IsSet(unsigned char value) { return value == 0xFF; }

std::optional<Line> FindLongestLineInColumn(cv::Mat column) {
  std::optional<Line> current_line;
  std::optional<Line> longest_line;

  if (IsSet(column.at<unsigned char>(0, 0))) {
    current_line = Line{0, 0};
  }

  for (int i = 1; i < column.rows; i++) {
    auto prev = IsSet(column.at<unsigned char>(i - 1, 0));
    auto current = IsSet(column.at<unsigned char>(i, 0));

    if (!prev && current) {
      current_line = Line{i, i};
    }

    if (prev && current) {
      current_line->end = i;
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

}  // namespace

std::optional<utils::RectRRf> FindLargestCrop(cv::Mat mask) {
  Line invalid_line = {mask.rows - 1, 0};
  std::vector<Line> lines(mask.cols);
  for (int i = 0; i < mask.cols; i++) {
    auto longest_line = FindLongestLineInColumn(mask.col(i));
    lines[i] = longest_line.value_or(invalid_line);
  }

  auto half_size = mask.cols / 2;
  auto current_rect = utils::RectPPi{{half_size, lines[half_size].start},
                                     {half_size, lines[half_size].end}};
  auto largest_rect = current_rect;

  for (int i = 0; i < half_size; i++) {
    auto left = half_size - i;
    auto right = half_size + i;
    auto left_line = lines[left];
    auto right_line = lines[right];

    current_rect.start[0] = left;
    current_rect.end[0] = right;

    if (int top = std::max(left_line.start, right_line.start);
        top > current_rect.start[1]) {
      current_rect.start[1] = top;
    }

    if (int bottom = std::min(left_line.end, right_line.end);
        bottom < current_rect.end[1]) {
      current_rect.end[1] = bottom;
    }

    if (utils::Area(current_rect) > utils::Area(largest_rect)) {
      largest_rect = current_rect;
    }
  }

  auto image_size = utils::Point2i{mask.cols, mask.rows};
  return Rect(largest_rect.start / image_size, largest_rect.end / image_size);
}

}  // namespace xpano::algorithm::crop
