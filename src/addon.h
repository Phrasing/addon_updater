#ifndef ADDON_H
#define ADDON_H
#pragma once

namespace addon_updater {
namespace tukui_fields {
constexpr auto kField_Id = "id";
constexpr auto kField_Name = "name";
constexpr auto kField_SmallDesc = "small_desc";
constexpr auto kField_Author = "author";
constexpr auto kField_Version = "version";
constexpr auto kField_ScreenshotUrl = "screenshot_url";
constexpr auto kField_DownloadUrl = "url";
constexpr auto kField_Category = "category";
constexpr auto kField_Downloads = "downloads";
constexpr auto kField_LastUpdate = "lastupdate";
constexpr auto kField_Patch = "patch";
constexpr auto kField_WebUrl = "web_url";
constexpr auto kField_LastDownload = "last_download";
constexpr auto kField_ChangeLog = "changelog";
constexpr auto kField_DonateUrl = "donate_url";
}

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
  bool Deserialize(const rj::Value::ConstObject& object);
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
  bool Deserialize(const rj::Value::ConstObject& object);
  std::string display_name;
  std::string file_name;
  std::string download_url;
  std::string game_version_flavor;
  uint32_t release_type;
  bool is_alternate;
};

namespace curse_fields {

constexpr auto kField_Id = "id";
constexpr auto kField_Name = "name";
constexpr auto kField_Authors = "authors";
constexpr auto kField_Attachments = "attachments";
constexpr auto kField_WebsiteUrl = "websiteUrl";
constexpr auto kField_GameId = "gameId";
constexpr auto kField_Summary = "summary";
constexpr auto kField_DefaultFileId = "defaultFileId";
constexpr auto kField_DownloadCount = "downloadCount";
constexpr auto kField_LatestFiles = "latestFiles";
constexpr auto kField_Categories = "categories";
constexpr auto kField_Status = "status";
constexpr auto kField_PrimaryCategoryId = "primaryCategoryId";
constexpr auto kField_CategorySection = "categorySection";
constexpr auto kField_Slug = "slug";
constexpr auto kField_GameVersionLatestFiles = "gameVersionLatestFiles";
constexpr auto kField_IsFeatured = "isFeatured";
constexpr auto kField_PopularityScore = "popularityScore";
constexpr auto kField_GamePopularityRank = "gamePopularityRank";
constexpr auto kField_PrimaryLanguage = "primaryLanguage";
constexpr auto kField_GameSlug = "gameSlug";
constexpr auto kField_GameName = "gameName";
constexpr auto kField_PortalName = "portalName";
constexpr auto kField_DateModified = "dateModified";
constexpr auto kField_DateCreated = "dateCreated";
constexpr auto kField_DateReleased = "dateReleased";
constexpr auto kField_IsAvailable = "isAvailable";
constexpr auto kField_IsExperimental = "isExperimental";

namespace attachments {
constexpr auto kField_Id = "id";
constexpr auto kField_ProjectId = "projectId";
constexpr auto kField_ThumbnailUrl = "thumbnailUrl";
constexpr auto kField_Url = "url";
constexpr auto kField_Description = "description";
constexpr auto kField_IsDefault = "isDefault";
}  // namespace attachments

}

enum class AddonType { kTukui, kCurse };
enum class AddonReleaseType { kStable = 1, kBeta = 2, kAlpha = 3 };
enum class AddonFlavor { kRetail, kRetailPtr, kClassic, kClassicPtr, kBeta };

inline std::string FlavorToString(AddonFlavor flavor) {
  switch (flavor) {
    case AddonFlavor::kRetail:
      return "wow_retail";
    case AddonFlavor::kClassic:
      return "wow_classic";
    default:
      return {};
  }
}

struct AddonThumbnail {
  uint8_t* pixels;
  size_t pixels_size;
  int width;
  int height;
  int channels;

  bool is_loaded = false;
  bool in_progress = false;
  bool is_uploaded = false;
};

struct InstalledAddon;

struct Slug {
  bool Deserialize(const rj::Value::ConstMemberIterator& iterator);
  std::string slug_name;
  std::string addon_name;
};

struct Addon {
  bool Deserialize(const rj::Value::ConstObject& object);
  InstalledAddon Install() const;

  bool operator==(const Addon& addon) const { return id == addon.id; }

  int32_t id;
  std::string name;
  std::string hash;
  std::string screenshot_url;
  std::string download_url;
  std::string slug;
  std::string author;
  std::string description;
  std::string readable_version;
  std::string stripped_version;

  int64_t numeric_version;

  bool is_ignored = false;

  DownloadStatus download_status;
  AddonThumbnail thumbnail;
  AddonFlavor flavor;
  AddonType type;
};

struct InstalledAddon : Addon {
  InstalledAddon& operator=(const Addon& addon);
  bool operator==(const InstalledAddon& addon) const {
    return this->id == addon.id;
  }
  void Serialize(rj::PrettyWriter<rj::StringBuffer>* writer) const;

  void Uninstall();
  bool Update();

  bool up_to_date = true;
  std::vector<std::string> directories;
};

struct InstalledAddonHash {
  size_t operator()(const InstalledAddon& addon) const {
    return static_cast<size_t>(addon.id);
  }
};

using LatestFiles = std::vector<CurseLatestFile>;
using Addons = std::vector<Addon>;
using Slugs = std::vector<Slug>;
using InstalledAddons = std::vector<InstalledAddon>;

bool DetectInstalledAddons(std::string_view addons_path, AddonFlavor flavor,
                           const Slugs& slugs, const Addons& addons,
                           InstalledAddons& installed_addons);

bool DeserializeAddonSlugs(std::string_view json, Slugs* slugs);

bool DeserializeAddons(std::string_view json, AddonType addon_type,
                       AddonFlavor addon_flavor, Addons* addons);

}  // namespace addon_updater

#endif  // !ADDON_H
