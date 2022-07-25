#include "thumbnail_pane.h"

#include <algorithm>
#include <functional>
#include <numeric>
#include <vector>

#include <imgui.h>
#include <opencv2/core.hpp>
#include <SDL.h>

#include "action.h"
#include "constants.h"
#include "image.h"

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

ThumbnailPane::ThumbnailPane(SDL_Renderer *renderer) : renderer_(renderer) {}

void ThumbnailPane::Load(const std::vector<Image> &images) {
  int num_images = static_cast<int>(images.size());

  int skip = kPreviewSize;

  int side = 0;
  while (side * side < num_images) {
    side++;
  }
  int size = side * skip;

  auto type = images[0].GetPreview().type();
  cv::Mat atlas{cv::Size(size, size), type};

  auto coord_uv = [size](int coord_px) {
    return static_cast<float>(coord_px) / static_cast<float>(size);
  };

  for (int i = 0; i < images.size(); i++) {
    int u_coord = (i % side) * skip;
    int v_coord = (i / side) * skip;

    const auto preview = images[i].GetPreview();
    preview.copyTo(atlas(cv::Rect(u_coord, v_coord, skip, skip)));
    Coord coord{ImVec2(coord_uv(u_coord), coord_uv(v_coord)),
                ImVec2(coord_uv(u_coord + skip), coord_uv(v_coord + skip)),
                images[i].GetAspect(), i};
    coords_.emplace_back(coord);
  }

  tex_.reset(SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_BGR24,
                               SDL_TEXTUREACCESS_STATIC, size, size));

  SDL_UpdateTexture(tex_.get(), nullptr, atlas.data,
                    static_cast<int>(atlas.step1()));
  SDL_Log("Success Atlas!");
}

bool ThumbnailPane::Loaded() const { return !coords_.empty(); }

Action ThumbnailPane::Draw() {
  ImGui::Begin("Images", nullptr, ImGuiWindowFlags_AlwaysHorizontalScrollbar);
  Action action{};

  if (scroll_.empty()) {
    scroll_.resize(coords_.size());
  }

  ImVec2 available_size = ImGui::GetContentRegionAvail();
  float thumbnail_height =
      available_size.y - 2 * ImGui::GetStyle().FramePadding.y;

  for (const auto &coord : coords_) {
    ImGui::PushID(coord.id);
    hover_checker_.SetColor(coord.id);
    float scroll_pre = ImGui::GetCursorPosX();
    if (ImGui::ImageButton(
            tex_.get(),
            ImVec2(thumbnail_height * coord.aspect, thumbnail_height),
            coord.uv0, coord.uv1)) {
      if (io_.KeyCtrl && hover_checker_.AllowsMofication()) {
        action = {ActionType::kModifyPano, coord.id};
      } else {
        action = {ActionType::kShowImage, coord.id};
      }
    }
    hover_checker_.ResetColor(coord.id, io_.KeyCtrl);
    ImGui::PopID();
    ImGui::SameLine();
    scroll_[coord.id] = (scroll_pre + ImGui::GetCursorPosX()) / 2.0f;
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
  auto find_scroll = [this](int img_id) {
    auto iter = std::find_if(
        coords_.begin(), coords_.end(),
        [img_id](const Coord &coord) { return coord.id == img_id; });
    return iter != coords_.end() ? scroll_[iter->id] : 0.0f;
  };

  float scroll = std::transform_reduce(ids.begin(), ids.end(), 0.0f,
                                       std::plus<>(), find_scroll) /
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
