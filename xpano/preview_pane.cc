#include "preview_pane.h"

#include <imgui.h>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <SDL.h>

#include "constants.h"
#include "image.h"

namespace xpano {

namespace {
[[nodiscard]] float GetAspect(const cv::Mat &image_data) {
  auto width = static_cast<float>(image_data.size[1]);
  auto height = static_cast<float>(image_data.size[0]);
  return width / height;
}
}  // namespace

PreviewPane::PreviewPane(SDL_Renderer *renderer) : renderer_(renderer){};

void PreviewPane::Load(cv::Mat image) {
  if (!tex_) {
    tex_.reset(SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_BGR24,
                                 SDL_TEXTUREACCESS_STATIC, kLoupeSize,
                                 kLoupeSize));
  }

  int larger_dim = image.size[0] > image.size[1] ? 0 : 1;

  cv::Mat resized;
  ImVec2 coord_uv;
  if (image.size[larger_dim] > kLoupeSize) {
    cv::Size size;
    float aspect = GetAspect(image);
    if (aspect >= 1.0f) {
      size = cv::Size(kLoupeSize, static_cast<int>(kLoupeSize / aspect));
      coord_uv = ImVec2(1.0f, 1.0f / aspect);
    } else {
      size = cv::Size(static_cast<int>(kLoupeSize * aspect), kLoupeSize);
      coord_uv = ImVec2(1.0f * aspect, 1.0f);
    }
    cv::resize(image, resized, size, 0, 0, cv::INTER_AREA);
  } else {
    resized = image;
    coord_uv = ImVec2(static_cast<float>(image.size[1]) / kLoupeSize,
                      static_cast<float>(image.size[0]) / kLoupeSize);
  }

  SDL_Rect target{0, 0, resized.size[1], resized.size[0]};
  SDL_UpdateTexture(tex_.get(), &target, resized.data,
                    static_cast<int>(resized.step1()));

  coord_ = Coord{ImVec2(0.0f, 0.0f), coord_uv, 1.0f, 0};
}

void PreviewPane::Draw() {
  ImGui::Begin("Preview", nullptr,
               ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
  if (tex_) {
    ImVec2 available_size = ImGui::GetContentRegionAvail();
    float aspect = coord_.uv1.x / coord_.uv1.y;

    ImVec2 size;
    if (available_size.x / available_size.y < aspect) {
      size = ImVec2(available_size.x, available_size.x / aspect);
    } else {
      size = ImVec2(available_size.y * aspect, available_size.y);
    }

    ImGui::Image(tex_.get(), size, coord_.uv0, coord_.uv1);
  }
  ImGui::End();
}

}  // namespace xpano
