#ifndef ADDON_UPDATER_THUMBNAIL_CACHE
#define ADDON_UPDATER_THUMBNAIL_CACHE
#pragma once

namespace addon_updater {

struct Thumbnail {
  int32_t id;
  uint8_t* input_data;
  std::string encoded_data;
  size_t data_length;
  int32_t width;
  int32_t height;
  int32_t channels;
};

using ThumbnailCache = std::vector<Thumbnail>;

void SerializeThumbnailCache(const ThumbnailCache& thumbnail_cache);
bool DeserializeThumbnailCache(ThumbnailCache* thumbnail_cache);

}  // namespace addon_updater

#endif  // !ADDON_UPDATER_THUMBNAIL_CACHE
