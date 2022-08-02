#include "gui/preview_pane.h"

#include <algorithm>
#include <cmath>
#include <numeric>

#include <imgui.h>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <SDL.h>

#include "constants.h"
#include "utils/vec.h"
#include "utils/vec_converters.h"
namespace xpano::gui {

PreviewPane::PreviewPane(SDL_Renderer *renderer) : renderer_(renderer) {
  std::iota(zoom_.begin(), zoom_.end(), 0.0f);
  std::transform(zoom_.begin(), zoom_.end(), zoom_.begin(),
                 [](float exp) { return std::pow(kZoomFactor, exp); });
};

float PreviewPane::Zoom() const { return zoom_[zoom_id_]; }

bool PreviewPane::IsZoomed() const { return zoom_id_ != 0; }

void PreviewPane::ZoomIn() {
  if (zoom_id_ < kZoomLevels - 1) {
    zoom_id_++;
  }
}

void PreviewPane::ZoomOut() {
  if (zoom_id_ > 0) {
    zoom_id_--;
  }
}

void PreviewPane::Load(cv::Mat image) {
  auto texture_size = utils::Vec2i{kLoupeSize};
  if (!tex_) {
    tex_.reset(SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_BGR24,
                                 SDL_TEXTUREACCESS_STATIC, texture_size[0],
                                 texture_size[1]));
  }

  int larger_dim = image.size[0] > image.size[1] ? 0 : 1;

  cv::Mat resized;
  utils::Ratio2f coord_uv;
  if (image.size[larger_dim] > kLoupeSize) {
    if (float aspect = utils::ToIntVec(image.size).Aspect(); aspect >= 1.0f) {
      coord_uv = {1.0f, 1.0f / aspect};
    } else {
      coord_uv = {1.0f * aspect, 1.0f};
    }
    auto size = utils::ToIntVec(texture_size * coord_uv);
    cv::resize(image, resized, utils::CvSize(size), 0, 0, cv::INTER_AREA);
  } else {
    resized = image;
    coord_uv = utils::ToIntVec(image.size) / texture_size;
  }
  auto target =
      utils::SdlRect(utils::Point2i{0}, utils::ToIntVec(resized.size));
  SDL_UpdateTexture(tex_.get(), &target, resized.data,
                    static_cast<int>(resized.step1()));
  tex_coord_ = coord_uv;
}

void PreviewPane::Draw() {
  ImGui::Begin("Preview");
  if (tex_) {
    auto available_size = utils::ToVec(ImGui::GetContentRegionAvail());
    auto mid =
        utils::ToPoint(ImGui::GetCursorScreenPos()) + available_size / 2.0f;

    float image_aspect = tex_coord_.Aspect();
    auto image_size =
        available_size.Aspect() < image_aspect
            ? utils::Vec2f{available_size[0], available_size[0] / image_aspect}
            : utils::Vec2f{available_size[1] * image_aspect, available_size[1]};

    auto p_min = mid - image_size / 2.0f;
    auto p_max = mid + image_size / 2.0f;
    if (IsZoomed()) {
      p_min = utils::ToPoint(ImGui::GetCursorScreenPos()) +
              available_size * screen_offset_ -
              image_size * image_offset_ * Zoom();
      p_max = p_min + image_size * Zoom();
    }

    ImGui::GetWindowDrawList()->AddImage(
        tex_.get(), utils::ImVec(p_min), utils::ImVec(p_max),
        ImVec2(0.0f, 0.0f), utils::ImVec(tex_coord_));

    if (ImGui::IsWindowHovered()) {
      bool mouse_clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
      bool mouse_dragging = ImGui::IsMouseDragging(ImGuiMouseButton_Left);
      float mouse_wheel = ImGui::GetIO().MouseWheel;

      if (mouse_clicked || mouse_dragging || mouse_wheel != 0) {
        auto window_start = utils::ToPoint(ImGui::GetCursorScreenPos());
        auto mouse_pos = utils::ToPoint(ImGui::GetMousePos());
        screen_offset_ = (mouse_pos - window_start) / available_size;
        if (!mouse_dragging) {
          image_offset_ = (mouse_pos - p_min) / Zoom() / image_size;
        }
      }
      if (mouse_wheel > 0) {
        ZoomIn();
      }
      if (mouse_wheel < 0) {
        ZoomOut();
      }
    }
  }
  ImGui::End();
}

}  // namespace xpano::gui
