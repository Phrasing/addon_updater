// clang-format off
#include "pch.h"
#include "thumbnail_cache.h"
#include "rapidjson_util.h"
#include "base64.h"

// clang-format on

using namespace beast::detail;

namespace addon_updater {
namespace {
constexpr auto kThumbnailId = "id";
constexpr auto kThumbnailWidth = "width";
constexpr auto kThumbnailHeight = "height";
constexpr auto kThumbnailChannels = "channels";
constexpr auto kThumbnailData = "thumbnail_data";
constexpr auto kAddonCacheFile = "thumbnail_cache.json";

/*std::string base64_encode(std::uint8_t const* data, std::size_t len) {
  std::string dest;
  dest.resize(base64::encoded_size(len));
  dest.resize(base64::encode(&dest[0], data, len));
  return dest;
}

inline std::string base64_encode(std::string const& s) {
  return base64_encode(reinterpret_cast<std::uint8_t const*>(s.data()),
                       s.size());
}

std::string base64_decode(std::string const& data) {
  std::string dest;
  dest.resize(base64::decoded_size(data.size()));
  auto const result = base64::decode(&dest[0], data.data(), data.size());
  dest.resize(result.first);
  return dest;
}*/

bool DeserializeThumbnail(const rj::Value::ConstObject& object,
                          Thumbnail* thumbnail) {
  thumbnail->id =
      rj_util::GetField<uint32_t>(object, kThumbnailId, rj::kNumberType);

  thumbnail->width =
      rj_util::GetField<uint32_t>(object, kThumbnailWidth, rj::kNumberType);
  thumbnail->height =
      rj_util::GetField<uint32_t>(object, kThumbnailHeight, rj::kNumberType);
  thumbnail->channels =
      rj_util::GetField<uint32_t>(object, kThumbnailChannels, rj::kNumberType);

  const auto encoded_thumbnail =
      std::move(rj_util::GetStringDef(object, kThumbnailData));

  auto s = base64_decode(encoded_thumbnail);

  //  std::vector<uint8_t>
  //  buffer(base64::decoded_size(encoded_thumbnail.size()));

  thumbnail->input_data = reinterpret_cast<uint8_t*>(s.data());
  thumbnail->data_length = s.size();

  return true;
}

void SerializeThumbnail(rj::PrettyWriter<rj::StringBuffer>* writer,
                        const Thumbnail& thumbnail) {
  writer->StartObject();
  {
    writer->String("id");
    writer->Int(thumbnail.id);
    writer->String("thumbnail_data");
    writer->String(thumbnail.encoded_data);
    writer->String("width");
    writer->Int(thumbnail.width);
    writer->String("height");
    writer->Int(thumbnail.height);
    writer->String("channels");
    writer->Int(thumbnail.channels);
  }
  writer->EndObject();
}

}  // namespace

void SerializeThumbnailCache(const ThumbnailCache& thumbnail_cache) {
  rj::StringBuffer string_buffer;
  rj::PrettyWriter<rj::StringBuffer> writer(string_buffer);
  writer.StartArray();
  {
    for (const auto& thumbnail : thumbnail_cache) {
      SerializeThumbnail(&writer, thumbnail);
    }
  }
  writer.EndArray();

  const auto cache_file_path =
      std::filesystem::temp_directory_path().string() + kAddonCacheFile;

  auto cache_file = std::make_unique<std::fstream>(
      cache_file_path, std::ios::out | std::ios::trunc);

  if (!cache_file->is_open()) return;

  cache_file->write(string_buffer.GetString(), string_buffer.GetLength());
}

bool DeserializeThumbnailCache(ThumbnailCache* thumbnail_cache) {
  const auto directory =
      std::filesystem::temp_directory_path().string() + kAddonCacheFile;

  auto config_file = std::make_unique<std::ifstream>(directory);

  if (!config_file->good()) return false;

  const auto contents =
      std::string{std::istreambuf_iterator<char>((*config_file)),
                  std::istreambuf_iterator<char>()};

  rj::Document document{};
  document.Parse(contents.data());

  if (document.HasParseError()) {
    return false;
  }

  for (const auto* it = document.Begin(); it != document.End(); ++it) {
  }

  return false;
}

}  // namespace addon_updater