#pragma once

#include <vector>

#include <opencv2/core.hpp>

#include "xpano/algorithm/algorithm.h"
#include "xpano/algorithm/image.h"
#include "xpano/algorithm/stitcher_pipeline.h"
#include "xpano/gui/action.h"
#include "xpano/gui/panels/thumbnail_pane.h"

namespace xpano::gui {

void DrawProgressBar(algorithm::ProgressReport progress);

cv::Mat DrawMatches(const algorithm::Match& match,
                    const std::vector<algorithm::Image>& images);

Action DrawMatchesMenu(const std::vector<algorithm::Match>& matches,
                       const ThumbnailPane& thumbnail_pane, int highlight_id);

Action DrawPanosMenu(const std::vector<algorithm::Pano>& panos,
                     const ThumbnailPane& thumbnail_pane, int highlight_id);

Action DrawMenu(algorithm::CompressionOptions* compression_options,
                algorithm::LoadingOptions* loading_options,
                algorithm::MatchingOptions* matching_options);

void DrawWelcomeText();

}  // namespace xpano::gui
