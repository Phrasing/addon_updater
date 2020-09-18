// clang-format off
#include "pch.h"
#include "addon.h"
#include "rapidjson_util.h"
// clang-format on
 
namespace addon_updater {
namespace {
std::optional<CurseLatestFile> CheckForRelease(
    AddonReleaseType release_type, AddonFlavor addon_flavor,
    const LatestFilesVect& latest_files) {
  for (const auto& latest_file : latest_files) {
    const auto flavor = FlavorToString(addon_flavor);
    if (latest_file.release_type == static_cast<uint32_t>(release_type) &&
        !latest_file.is_alternate &&
        latest_file.game_version_flavor == flavor) {
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
    addon->readable_version = stable.value().display_name;
    addon->stripped_version =
        string_util::StripNonDigits(addon->readable_version);
    addon->numeric_version =
        string_util::StringToNumber(addon->stripped_version);
    addon->download_url = stable.value().download_url;
  } else if (auto beta = CheckForRelease(AddonReleaseType::kBeta, addon->flavor,
                                         latest_files);
             beta.has_value()) {
    addon->readable_version = beta.value().display_name;
    addon->stripped_version =
        string_util::StripNonDigits(addon->readable_version);
    addon->numeric_version =
        string_util::StringToNumber(addon->stripped_version);
    addon->download_url = beta.value().download_url;
  }

  return true;
}

bool DeserializeTukui(const rj::Value::ConstObject& object, Addon* addon) {
  return true;
}
}  // namespace

bool Addon::Deserialize(const rj::Value::ConstObject& object) {
  switch (this->type) {
    case AddonType::kCurse: {
      DeserializeCurse(object, this);
    } break;

    case AddonType::kTukui: {
      DeserializeTukui(object, this);
    } break;

    default:
      break;
  }
  return true;
}

InstalledAddon Addon::Install() const {
  auto installed_addon = InstalledAddon{};

  installed_addon.hash = this->hash;
  installed_addon.name = this->name;
  installed_addon.id = this->id;
  installed_addon.slug = this->slug;
  installed_addon.flavor = this->flavor;
  installed_addon.type = this->type;
  installed_addon.readable_version = this->readable_version;

  return installed_addon;
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
  writer->String(this->readable_version);
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

bool InstalledAddon::Update() { return false; }

bool DeserializeAddons(std::string_view json, AddonType addon_type,
                       AddonVect* addons) {
  rj::Document document{};
  document.Parse(json.data(), json.length());
  if (document.HasParseError()) {
    return false;
  }

  for (const auto* it = document.Begin(); it != document.End(); ++it) {
    Addon addon{};
    addon.type = addon_type;
    addon.flavor = AddonFlavor::kRetail;
    if (addon.Deserialize(it->GetObject())) {
      addons->push_back(addon);
    }
  }

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
  return true;
}
