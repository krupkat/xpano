#include "gui/thumbnail_pane.h"

#include <algorithm>
#include <functional>
#include <numeric>
#include <vector>

#include <imgui.h>
#include <opencv2/core.hpp>
#include <SDL.h>

#include "algorithm/image.h"
#include "constants.h"
#include "gui/action.h"
#include "utils/vec.h"
#include "utils/vec_converters.h"

namespace xpano::gui {

void HoverChecker::SetColor(int img_id) {
  bool highlighted = std::find(highlighted_ids_.begin(), highlighted_ids_.end(),
                               img_id) != highlighted_ids_.end();

  if (WasHovered(img_id) && allow_modification_) {
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, highlighted
                                                      ? ImVec4(0.75, 0, 0, 1)
                                                      : ImVec4(0, 0.75, 0, 1));
    styles_pushed_ = 1;
    ResetHover();
  } else if (highlighted) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0.75, 0, 1));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 1, 0, 1));
    styles_pushed_ = 2;
  }
}

void HoverChecker::ResetColor(int img_id, bool ctrl_pressed) {
  if (ImGui::IsItemHovered() && ctrl_pressed) {
    RecordHover(img_id);
  }
  while (styles_pushed_ > 0) {
    ImGui::PopStyleColor();
    styles_pushed_--;
  }
}

void HoverChecker::Highlight(const std::vector<int> &ids,
                             bool allow_modification) {
  highlighted_ids_ = ids;
  allow_modification_ = allow_modification;
}

bool HoverChecker::AllowsMofication() const { return allow_modification_; }

bool HoverChecker::WasHovered(int img_id) const { return hover_id_ == img_id; }
void HoverChecker::RecordHover(int img_id) { hover_id_ = img_id; }
void HoverChecker::ResetHover() { hover_id_ = -1; }

void AutoScroller::SetNeedsRescroll() {
  needs_rescroll_ = true;
  scroll_ratio_ = ImGui::GetScrollX() / ImGui::GetScrollMaxX();
}

bool AutoScroller::NeedsRescroll() const { return needs_rescroll_; }

void AutoScroller::Rescroll() {
  ImGui::SetScrollX(ImGui::GetScrollMaxX() * scroll_ratio_);
  needs_rescroll_ = false;
}

ResizeChecker::ResizeChecker(int delay) : delay_(delay) {}

ResizeChecker::Status ResizeChecker::Check(ImVec2 window_size) {
  if (window_size.y != window_size_.y || window_size.x != window_size_.x ||
      (resizing_streak_ > 0 && resizing_streak_ < delay_)) {
    window_size_ = window_size;
    resizing_streak_++;
    return Status::kResizing;
  }
  if (resizing_streak_ >= delay_) {
    resizing_streak_ = 0;
    return Status::kResized;
  }
  return Status::kIdle;
}

ThumbnailPane::ThumbnailPane(SDL_Renderer *renderer) : renderer_(renderer) {}

void ThumbnailPane::Load(const std::vector<algorithm::Image> &images) {
  int num_images = static_cast<int>(images.size());
  auto thumbnail_size = utils::Vec2i{kPreviewSize};
  int side = 0;
  while (side * side < num_images) {
    side++;
  }
  auto size = thumbnail_size * side;
  auto type = images[0].GetPreview().type();
  cv::Mat atlas{utils::CvSize(size), type};
  for (int i = 0; i < images.size(); i++) {
    auto tex_coord = thumbnail_size * utils::Ratio2i{i % side, i / side};
    const auto preview = images[i].GetPreview();
    preview.copyTo(
        atlas(utils::CvRect(utils::Point2i{0} + tex_coord, thumbnail_size)));
    Coord coord{tex_coord / size, (tex_coord + thumbnail_size) / size,
                images[i].GetAspect()};
    coords_.emplace_back(coord);
  }
  scroll_.resize(coords_.size());
  tex_.reset(SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_BGR24,
                               SDL_TEXTUREACCESS_STATIC, size[0], size[1]));
  SDL_UpdateTexture(tex_.get(), nullptr, atlas.data,
                    static_cast<int>(atlas.step1()));
  SDL_Log("Success Atlas!");
}

bool ThumbnailPane::Loaded() const { return !coords_.empty(); }

Action ThumbnailPane::Draw() {
  ImGui::Begin("Images", nullptr, ImGuiWindowFlags_AlwaysHorizontalScrollbar);
  Action action{};

  if (auto_scroller_.NeedsRescroll()) {
    auto_scroller_.Rescroll();
  }

  if (auto window_status = resize_checker_.Check(ImGui::GetWindowSize());
      window_status != ResizeChecker::Status::kResizing) {
    thumbnail_height_ =
        ImGui::GetContentRegionAvail().y - 2 * ImGui::GetStyle().FramePadding.y;
    if (window_status == ResizeChecker::Status::kResized) {
      auto_scroller_.SetNeedsRescroll();
    }
  }

  for (int coord_id = 0; coord_id < coords_.size(); coord_id++) {
    ImGui::PushID(coord_id);
    hover_checker_.SetColor(coord_id);
    float scroll_pre = ImGui::GetCursorPosX();
    const auto &coord = coords_[coord_id];
    if (ImGui::ImageButton(
            tex_.get(),
            ImVec2(thumbnail_height_ * coord.aspect, thumbnail_height_),
            utils::ImVec(coord.uv0), utils::ImVec(coord.uv1))) {
      if (io_.KeyCtrl && hover_checker_.AllowsMofication()) {
        action = {ActionType::kModifyPano, coord_id};
      } else {
        action = {ActionType::kShowImage, coord_id};
      }
    }
    hover_checker_.ResetColor(coord_id, io_.KeyCtrl);
    ImGui::PopID();
    ImGui::SameLine();
    scroll_[coord_id] = (scroll_pre + ImGui::GetCursorPosX()) / 2.0f;
  }
  ImGui::End();

  return action;
}

void ThumbnailPane::SetScrollX(int img_id) {
  SetScrollX(std::vector<int>({img_id}));
}
void ThumbnailPane::SetScrollX(int id1, int id2) {
  SetScrollX(std::vector<int>({id1, id2}));
}
void ThumbnailPane::SetScrollX(const std::vector<int> &ids) {
  float scroll =
      std::transform_reduce(ids.begin(), ids.end(), 0.0f, std::plus<>(),
                            [this](int index) { return scroll_[index]; }) /
      static_cast<float>(ids.size());

  ImGui::Begin("Images");
  ImGui::SetScrollFromPosX(ImGui::GetCursorStartPos().x + scroll);
  ImGui::End();
}

void ThumbnailPane::Highlight(int img_id) {
  Highlight(std::vector<int>({img_id}));
}
void ThumbnailPane::Highlight(int id1, int id2) {
  Highlight(std::vector<int>({id1, id2}));
}
void ThumbnailPane::Highlight(const std::vector<int> &ids,
                              bool allow_modification) {
  hover_checker_.Highlight(ids, allow_modification);
}

void ThumbnailPane::Reset() {
  tex_.reset(nullptr);
  coords_.resize(0);
  scroll_.resize(0);
  hover_checker_ = HoverChecker{};
}

}  // namespace xpano::gui
