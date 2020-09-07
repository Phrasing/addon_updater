#ifndef ADDON_H
#define ADDON_H
#pragma once

namespace addon_updater {

namespace curse_structs {

struct CurseAuthor {
  std::string name;
  std::string url;
  std::string project_title;
  uint32_t project_id;
  uint32_t id;
  int32_t project_title_id;
  uint32_t user_id;
  uint32_t twitch_id;
};

struct CurseAttachment {
  std::string description;
  std::string thumbnail_url;
  std::string title;
  std::string url;
  bool is_default;
  uint32_t id;
  uint32_t project_id;
  uint32_t status;
};

struct CurseGameVersionLatestFile {
  int32_t project_file_id;
  int32_t file_type;
  std::string project_file_name;
  std::string game_version;
  std::string game_version_flavor;
};

struct CurseLatestFile {
  std::string display_name;
  std::string file_name;
  std::string download_url;
  std::string game_version_flavor;
  uint32_t release_type;
  bool is_alternate;
};

}  // namespace curse_structs

enum class AddonType { kTukui, kCurse };
enum class AddonFlavor { kRetail, kRetailPtr, kClassic, kClassicPtr, kBeta };

struct AddonThumbnail {
  void* pixels;
  int32_t width;
  int32_t height;
  int32_t channels;
};

struct Addon {
  bool Deserialize(const rj::Value::ConstObject& object);
  bool Install();
  bool operator==(const Addon& addon) { return id == addon.id; }

  int32_t id;
  std::string name;
  std::string hash;
  std::string screenshot_url;
  std::string download_url;
  std::string slug;
  std::string author;
  std::string description;
  std::string readable_version;

  AddonThumbnail thumbnail;
  AddonFlavor flavor;
  AddonType type;
};

struct InstalledAddon : Addon {
  std::string Serialize() const;

  void Uninstall();
  bool Update();

  bool up_to_date;
  std::unordered_set<std::string> directories;
};

}  // namespace addon_updater

#endif  // !ADDON_H
