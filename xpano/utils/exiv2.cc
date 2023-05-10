#include "xpano/utils/exiv2.h"

#include <exiv2/exiv2.hpp>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

#include "xpano/version_fmt.h"

namespace xpano::utils::exiv2 {

namespace {
void AddSoftwareTag(Exiv2::ExifData& exif_data) {
  exif_data["Exif.Image.Software"] =
      fmt::format("Xpano {}", version::Current());
}

}  // namespace

void CreateExif(const std::filesystem::path& from_path,
                const std::filesystem::path& to_path) {
  try {
    auto read_img = Exiv2::ImageFactory::open(from_path.string());
    read_img->readMetadata();

    auto write_img = Exiv2::ImageFactory::open(to_path.string());
    write_img->setExifData(read_img->exifData());

    AddSoftwareTag(write_img->exifData());
    write_img->writeMetadata();
  } catch (const Exiv2::Error&) {
    spdlog::warn("Could not copy Exif data from {} to {}", from_path.string(),
                 to_path.string());
  }
}

}  // namespace xpano::utils::exiv2
