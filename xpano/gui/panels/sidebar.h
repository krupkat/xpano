// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <vector>

#include <opencv2/core.hpp>

#include "xpano/algorithm/algorithm.h"
#include "xpano/algorithm/image.h"
#include "xpano/gui/action.h"
#include "xpano/gui/panels/preview_pane.h"
#include "xpano/gui/panels/thumbnail_pane.h"
#include "xpano/pipeline/options.h"
#include "xpano/pipeline/stitcher_pipeline.h"

namespace xpano::gui {

void DrawProgressBar(pipeline::ProgressReport progress);

cv::Mat DrawMatches(const algorithm::Match& match,
                    const std::vector<algorithm::Image>& images);

Action DrawMatchesMenu(const std::vector<algorithm::Match>& matches,
                       const ThumbnailPane& thumbnail_pane, int highlight_id);

Action DrawPanosMenu(const std::vector<algorithm::Pano>& panos,
                     const ThumbnailPane& thumbnail_pane, int highlight_id);

Action DrawMenu(pipeline::Options* options, bool debug_enabled);

void DrawWelcomeTextPart1();

void DrawWelcomeTextPart2();

Action DrawImportActionButtons();

Action DrawActionButtons(ImageType image_type, int target_id,
                         algorithm::ProjectionType* projection_type);

}  // namespace xpano::gui
