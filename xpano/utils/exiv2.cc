// SPDX-FileCopyrightText: 2023 Tomas Krupka
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xpano/utils/exiv2.h"

#include <exiv2/exiv2.hpp>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

#include "xpano/constants.h"
#include "xpano/utils/path.h"
#include "xpano/utils/vec.h"
#include "xpano/version_fmt.h"

namespace xpano::utils::exiv2 {

namespace {

template <typename TValueType>
void UpdateTagIfExisting(Exiv2::ExifData& exif_data, const std::string& key,
                         TValueType value) {
  if (auto exif_datum = exif_data.findKey(Exiv2::ExifKey(key));
      exif_datum != exif_data.end()) {
    *exif_datum = value;
  }
}

void AddSoftwareTag(Exiv2::ExifData& exif_data) {
  exif_data["Exif.Image.Software"] =
      fmt::format("Xpano {}", version::Current());
}

void UpdateImageSize(Exiv2::ExifData& exif_data, const Vec2i& image_size) {
  UpdateTagIfExisting(exif_data, "Exif.Image.ImageWidth", image_size[0]);
  UpdateTagIfExisting(exif_data, "Exif.Image.ImageLength", image_size[1]);

  UpdateTagIfExisting(exif_data, "Exif.Photo.PixelXDimension", image_size[0]);
  UpdateTagIfExisting(exif_data, "Exif.Photo.PixelYDimension", image_size[1]);
}

void UpdateOrientation(Exiv2::ExifData& exif_data, int orientation) {
  UpdateTagIfExisting(exif_data, "Exif.Image.Orientation", orientation);
  UpdateTagIfExisting(exif_data, "Exif.Thumbnail.Orientation", orientation);
}

}  // namespace

void CreateExif(const std::filesystem::path& from_path,
                const std::filesystem::path& to_path, const Vec2i& image_size) {
  if (!path::IsMetadataExtensionSupported(from_path)) {
    spdlog::info("Reading metadata is not supported for {}",
                 from_path.string());
    return;
  }
  if (!path::IsMetadataExtensionSupported(to_path)) {
    spdlog::warn("Writing metadata is not supported for {}", to_path.string());
    return;
  }

  try {
    auto read_img = Exiv2::ImageFactory::open(from_path.string());
    read_img->readMetadata();

    auto write_img = Exiv2::ImageFactory::open(to_path.string());
    write_img->setExifData(read_img->exifData());

    AddSoftwareTag(write_img->exifData());
    UpdateImageSize(write_img->exifData(), image_size);
    UpdateOrientation(write_img->exifData(), kExifDefaultOrientation);

    write_img->writeMetadata();
  } catch (const Exiv2::Error&) {
    spdlog::warn("Could not copy Exif data from {} to {}", from_path.string(),
                 to_path.string());
  }
}

}  // namespace xpano::utils::exiv2
