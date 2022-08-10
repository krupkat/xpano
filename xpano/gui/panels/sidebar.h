#pragma once

#include "algorithm/stitcher_pipeline.h"
#include "gui/action.h"
#include "gui/panels/thumbnail_pane.h"

namespace xpano::gui {

void DrawProgressBar(algorithm::ProgressReport progress);

cv::Mat DrawMatches(const algorithm::Match& match,
                    const std::vector<algorithm::Image>& images);

Action DrawMatchesMenu(const std::vector<algorithm::Match>& matches,
                       const ThumbnailPane& thumbnail_pane, int highlight_id);

Action DrawPanosMenu(const std::vector<algorithm::Pano>& panos,
                     const ThumbnailPane& thumbnail_pane, int highlight_id);

Action DrawMenu();

}  // namespace xpano::gui
