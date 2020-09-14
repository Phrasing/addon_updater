// clang-format off
#include "pch.h"
#include "addon.h"
#include "rapidjson_util.h"
// clang-format on

namespace addon_updater {

bool DeserializeCurse(const rj::Value::ConstObject& object, Addon* addon) {
  addon->id = rj_util::GetField<uint32_t>(object, curse_structs::kField_Id,
                                          rj::kNumberType);
  addon->name =
      std::move(rj_util::GetStringDef(object, curse_structs::kField_Name));
  addon->description =
      std::move(rj_util::GetStringDef(object, curse_structs::kField_Summary));
  addon->slug =
      std::move(rj_util::GetStringDef(object, curse_structs::kField_Slug));
  addon->type = AddonType::kCurse;

  if (rj_util::HasMemberOfType(object, curse_structs::kField_Attachments,
                               rj::kArrayType)) {
    const auto array = object[curse_structs::kField_Attachments].GetArray();
    for (const auto* it = array.Begin(); it != array.End(); ++it) {
      if (!it->IsObject()) continue;
      curse_structs::CurseAttachment attachment{};
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
  return true;
}

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
    for (auto& directory : this->directories) {
      writer->String(directory);
    }
    writer->EndArray();
  }

  writer->EndObject();
}

void InstalledAddon::Uninstall() {
  for (auto& directory : directories) {
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
    if (addon.Deserialize(it->GetObject())) {
      addons->push_back(addon);
    }
  }

  return true;
}

}  // namespace addon_updater

bool addon_updater::curse_structs::CurseAttachment::Deserialize(
    const rj::Value::ConstObject& object) {
  this->id = rj_util::GetField<uint32_t>(
      object, curse_structs::attachments::kField_Id, rj::kNumberType);
  this->project_id = rj_util::GetField<uint32_t>(
      object, curse_structs::attachments::kField_ProjectId, rj::kNumberType);
  this->description = std::move(rj_util::GetStringDef(
      object, curse_structs::attachments::kField_Description));
  this->is_default =
      rj_util::GetBoolDef(object, curse_structs::attachments::kField_IsDefault);
  this->url = std::move(
      rj_util::GetStringDef(object, curse_structs::attachments::kField_Url));

  this->thumbnail_url = std::move(rj_util::GetStringDef(
      object, curse_structs::attachments::kField_ThumbnailUrl));

  return true;
}
