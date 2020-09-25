// clang-format off
#include "pch.h"
#include "addon.h"
#include "toc_parser.h"
#include "rapidjson_util.h"
#include "file.h"
#include "zip_file.h"
// clang-format on

namespace addon_updater {
namespace {
std::optional<CurseLatestFile> CheckForRelease(AddonReleaseType release_type,
                                               AddonFlavor addon_flavor,
                                               LatestFiles& latest_files) {
  for (auto& latest_file : latest_files) {
    const auto flavor = FlavorToString(addon_flavor);
    if (latest_file.release_type == static_cast<uint32_t>(release_type) &&
        !latest_file.is_alternate &&
        latest_file.game_version_flavor == flavor) {
      string_util::ReplaceAll(&latest_file.download_url, "edge", "media");

      return latest_file;
    }
  }
  return std::nullopt;
}

bool DeserializeCurse(const rj::Value::ConstObject& object, Addon* addon) {
  addon->id = rj_util::GetField<uint32_t>(object, curse_fields::kField_Id,
                                          rj::kNumberType);
  addon->name =
      std::move(rj_util::GetStringDef(object, curse_fields::kField_Name));
  addon->description =
      std::move(rj_util::GetStringDef(object, curse_fields::kField_Summary));
  addon->slug =
      std::move(rj_util::GetStringDef(object, curse_fields::kField_Slug));
  addon->type = AddonType::kCurse;

  std::vector<CurseLatestFile> latest_files{};
  if (rj_util::HasMemberOfType(object, curse_fields::kField_LatestFiles,
                               rj::kArrayType)) {
    const auto curse_latest_files =
        object[curse_fields::kField_LatestFiles].GetArray();
    for (const auto* it = curse_latest_files.Begin();
         it != curse_latest_files.End(); ++it) {
      if (!it->IsObject()) continue;

      CurseLatestFile latest_file{};
      if (latest_file.Deserialize(it->GetObject())) {
        latest_files.push_back(latest_file);
      }
    }
  }

  if (auto stable = CheckForRelease(AddonReleaseType::kStable, addon->flavor,
                                    latest_files);
      stable.has_value()) {
    addon->remote_version.readable_version = stable.value().display_name;
    addon->remote_version.stripped_version =
        string_util::StripNonDigits(addon->remote_version.readable_version);
    addon->remote_version.numeric_version =
        string_util::StringToNumber(addon->remote_version.stripped_version);
    addon->download_url = stable.value().download_url;
    addon->latest_file = *stable;
  } else if (auto beta = CheckForRelease(AddonReleaseType::kBeta, addon->flavor,
                                         latest_files);
             beta.has_value()) {
    addon->remote_version.readable_version = beta.value().display_name;
    addon->remote_version.stripped_version =
        string_util::StripNonDigits(addon->remote_version.readable_version);
    addon->remote_version.numeric_version =
        string_util::StringToNumber(addon->remote_version.stripped_version);
    addon->download_url = beta->download_url;
    addon->latest_file = *beta;
  }

  if (addon->download_url.empty()) return false;

  if (rj_util::HasMemberOfType(object, curse_fields::kField_Attachments,
                               rj::kArrayType)) {
    const auto curse_attachments =
        object[curse_fields::kField_Attachments].GetArray();
    for (const auto* it = curse_attachments.Begin();
         it != curse_attachments.End(); ++it) {
      if (!it->IsObject()) continue;
      CurseAttachment attachment{};
      if (attachment.Deserialize(it->GetObject()) && attachment.is_default) {
        string_util::ReplaceAll(&attachment.thumbnail_url, R"(/256)", R"(/64)");
        addon->screenshot_url = attachment.thumbnail_url;
        break;
      }
    }
  }

  return true;
}

bool DeserializeTukui(const rj::Value::ConstObject& object, Addon* addon) {
  addon->name =
      std::move(rj_util::GetStringDef(object, tukui_fields::kField_Name));
  addon->description =
      std::move(rj_util::GetStringDef(object, tukui_fields::kField_SmallDesc));
  addon->author =
      std::move(rj_util::GetStringDef(object, tukui_fields::kField_Author));
  addon->remote_version.readable_version =
      std::move(rj_util::GetStringDef(object, tukui_fields::kField_Version));
  addon->remote_version.stripped_version =
      string_util::StripNonDigits(addon->remote_version.readable_version);
  addon->remote_version.numeric_version =
      string_util::StringToNumber(addon->remote_version.stripped_version);
  addon->screenshot_url = std::move(
      rj_util::GetStringDef(object, tukui_fields::kField_ScreenshotUrl));
  addon->download_url = std::move(
      rj_util::GetStringDef(object, tukui_fields::kField_DownloadUrl));

  return true;
}
}  // namespace

bool Addon::Deserialize(const rj::Value::ConstObject& object) {
  bool result = false;
  switch (this->type) {
    case AddonType::kCurse: {
      result = DeserializeCurse(object, this);
    } break;

    case AddonType::kTukui: {
      result = DeserializeTukui(object, this);
    } break;

    default:
      break;
  }
  return result;
}

InstalledAddon Addon::Install() const {
  auto installed_addon = InstalledAddon{};

  installed_addon.hash = this->hash;
  installed_addon.name = this->name;
  installed_addon.id = this->id;
  installed_addon.slug = this->slug;
  installed_addon.flavor = this->flavor;
  installed_addon.type = this->type;
  installed_addon.local_version = this->remote_version;

  return installed_addon;
}

InstalledAddon& InstalledAddon::operator=(const Addon& addon) {
  this->author = addon.author;
  this->description = addon.description;
  this->download_status = addon.download_status;
  this->download_url = addon.download_url;
  this->flavor = addon.flavor;
  this->id = addon.id;
  this->slug = addon.slug;
  this->name = addon.name;
  this->thumbnail = addon.thumbnail;
  this->thumbnail.is_loaded = false;
  this->thumbnail.is_uploaded = false;
  this->thumbnail.pixels = nullptr;
  this->thumbnail.in_progress = false;
  this->screenshot_url = addon.screenshot_url;
  this->remote_version = addon.remote_version;
  return *this;
}

void InstalledAddon::Serialize(
    rj::PrettyWriter<rj::StringBuffer>* writer) const {
  writer->StartObject();
  writer->String("id");
  writer->Int(this->id);
  writer->String("name");
  writer->String(this->name);
  writer->String("download_url");
  writer->String(this->download_url);
  writer->String("slug");
  writer->String(this->slug);
  writer->String("checksum");
  writer->String(this->hash);
  writer->String("flavor");
  writer->Int(static_cast<int32_t>(this->flavor));
  writer->String("type");
  writer->Int(static_cast<int32_t>(this->type));
  writer->String("version");
  writer->String(this->local_version.readable_version);
  if (!this->directories.empty()) {
    writer->String("directories");
    writer->StartArray();
    for (const auto& directory : this->directories) {
      writer->String(directory);
    }
    writer->EndArray();
  }

  writer->EndObject();
}

void InstalledAddon::Uninstall() {
  for (const auto& directory : directories) {
    if (!std::filesystem::remove_all(directory)) {
      std::cerr << "Failed to remove directory " << directory << std::endl;
    }
  }
}

bool InstalledAddon::Update() {
  ClientFactory::GetInstance().NewAsyncClient()->Download(
      this->download_url,
      [&](const beast::error_code& ec, std::string_view response) {
        this->Uninstall();

        const auto buffer =
            std::vector<uint8_t>(response.begin(), response.end());

        auto zip_file = std::make_unique<ZipFile>(buffer);
        if (!zip_file->Good()) {
          std::fprintf(stderr, "Error: failed to open zip file %s\n",
                       this->name.c_str());
          return;
        }

        if (!zip_file->Unzip(this->addons_directory, &this->directories)) {
          std::fprintf(stderr, "Error: failed to extract zip file %s\n",
                       this->name.c_str());
        }

        this->up_to_date = true;
      },
      [&](const DownloadStatus& status) -> bool {
        if (this->download_status.state == RequestState::kStateCancel)
          return false;
        this->download_status = status;
        return true;
      });
  return false;
}

bool DetectInstalledAddons(std::string_view addons_path, AddonFlavor flavor,
                           const Slugs& slugs, const Addons& addons,
                           InstalledAddons& installed_addons) {
  if (!OsDirectoryExists(addons_path.data())) return false;

  for (auto& addon : std::filesystem::directory_iterator(addons_path)) {
    const auto addon_name = addon.path().filename().string();

    const auto is_addon =
        std::find_if(slugs.begin(), slugs.end(), [&](const Slug& slug) -> bool {
          return slug.addon_name == addon_name;
        });

    if (is_addon != slugs.end()) {
      InstalledAddon installed_addon{};
      installed_addon.addons_directory = addons_path;

      bool check_next = true;
      for (auto& installed_addon : installed_addons) {
        if (installed_addon.slug == is_addon->slug_name) {
          installed_addon.directories.push_back(addon.path().string());
          check_next = false;
        }
      }

      if (!check_next) continue;

      const auto toc_file = addon.path().string() + R"(\)" +
                            addon.path().filename().string() + ".toc";

      if (!OsFileExists(toc_file.c_str())) continue;

      TocParser toc_parser(toc_file);
      if (!toc_parser.Ok()) return false;

      const auto result = toc_parser.ParseTocFile();
      if (!result.has_value()) continue;

      if (result->readable_version.empty()) {
        continue;
      }

      const auto is_detected =
          std::find_if(addons.begin(), addons.end(),
                       [&is_addon](const Addon& addon) -> bool {
                         return addon.slug == is_addon->slug_name;
                       });

      if (is_detected != addons.end()) {
        installed_addon.local_version =
            AddonVersion{result->readable_version, result->stripped_version,
                         result->numeric_version};

        installed_addon.directories.push_back(addon.path().string());

        if (installed_addon.local_version.stripped_version.find(
                is_detected->remote_version.stripped_version) ==
            std::string::npos) {
          installed_addon.up_to_date = false;
        }

        installed_addon = *is_detected;
        installed_addons.push_back(installed_addon);
      }
    }
  }

  return true;
}

bool DeserializeAddonSlugs(std::string_view json, Slugs* slugs) {
  rj::Document doc;
  doc.Parse(json.data(), json.length());

  if (doc.HasParseError()) {
    return false;
  }

  for (rj::Value::ConstMemberIterator itr = doc.MemberBegin();
       itr != doc.MemberEnd(); ++itr) {
    Slug addon_slug{};
    if (addon_slug.Deserialize(itr)) {
      slugs->push_back(addon_slug);
    }
  }

  return true;
}

bool DeserializeAddons(std::string_view json, AddonType addon_type,
                       AddonFlavor addon_flavor, Addons* addons) {
  rj::Document document{};
  document.Parse(json.data(), json.length());
  if (document.HasParseError()) {
    return false;
  }

  for (const auto* it = document.Begin(); it != document.End(); ++it) {
    Addon addon{};
    addon.type = addon_type;
    addon.flavor = addon_flavor;
    if (addon.Deserialize(it->GetObject())) {
      addons->push_back(addon);
    }
  }

  return true;
}

bool Slug::Deserialize(const rj::Value::ConstMemberIterator& iterator) {
  if (!(iterator->name.IsString() && iterator->value.IsArray() &&
        !iterator->value.GetArray().Empty())) {
    return false;
  }

  this->addon_name = std::move(iterator->name.GetString());
  this->slug_name = std::move(iterator->value.GetArray()[0].GetString());

  return true;
}

bool CurseModule::Deserialize(const rj::Value::ConstObject& object) {
  this->folder_name = std::move(rj_util::GetStringDef(object, "foldername"));
  this->finger_print =
      rj_util::GetField<size_t>(object, "fingerprint", rj::kNumberType);

  this->type = rj_util::GetField<int>(object, "type", rj::kNumberType);
  return true;
}

}  // namespace addon_updater

bool addon_updater::CurseAttachment::Deserialize(
    const rj::Value::ConstObject& object) {
  this->id = rj_util::GetField<uint32_t>(
      object, curse_fields::attachments::kField_Id, rj::kNumberType);
  this->project_id = rj_util::GetField<uint32_t>(
      object, curse_fields::attachments::kField_ProjectId, rj::kNumberType);
  this->description = std::move(rj_util::GetStringDef(
      object, curse_fields::attachments::kField_Description));
  this->is_default =
      rj_util::GetBoolDef(object, curse_fields::attachments::kField_IsDefault);
  this->url = std::move(
      rj_util::GetStringDef(object, curse_fields::attachments::kField_Url));

  this->thumbnail_url = std::move(rj_util::GetStringDef(
      object, curse_fields::attachments::kField_ThumbnailUrl));

  return true;
}

bool addon_updater::CurseLatestFile::Deserialize(
    const rj::Value::ConstObject& object) {
  this->display_name = std::move(rj_util::GetStringDef(object, "displayName"));
  this->game_version_flavor =
      std::move(rj_util::GetStringDef(object, "gameVersionFlavor"));
  this->download_url = std::move(rj_util::GetStringDef(object, "downloadUrl"));
  this->file_name = std::move(rj_util::GetStringDef(object, "fileName"));
  this->is_alternate = rj_util::GetBoolDef(object, "isAlternate");
  this->release_type =
      rj_util::GetField<uint32_t>(object, "releaseType", rj::kNumberType);

  if (rj_util::HasMemberOfType(object, curse_fields::kField_Modules,
                               rj::kArrayType)) {
    const auto addon_modules = object[curse_fields::kField_Modules].GetArray();
    for (const auto* it = addon_modules.Begin(); it != addon_modules.End();
         ++it) {
      if (!it->IsObject()) continue;

      CurseModule module{};
      if (module.Deserialize(std::move(it->GetObject()))) {
        this->modules.push_back(module);
      }
    }
  }

  return true;
}
