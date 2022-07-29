#include "gui/preview_pane.h"

#include <imgui.h>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <SDL.h>

#include "constants.h"
#include "utils/vec.h"
#include "utils/vec_converters.h"

namespace xpano::gui {

PreviewPane::PreviewPane(SDL_Renderer *renderer) : renderer_(renderer){};

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
    ImGui::GetWindowDrawList()->AddImage(
        tex_.get(), utils::ImVec(p_min), utils::ImVec(p_max),
        ImVec2(0.0f, 0.0f), utils::ImVec(tex_coord_));
  }
  ImGui::End();
}

}  // namespace xpano::gui
