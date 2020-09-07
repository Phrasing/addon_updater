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

std::vector<Addon> DeserializeAddons(std::string_view json,
                                     AddonType addon_type) {
  rj::Document document{};
  document.Parse(json.data(), json.length());
  if (document.HasParseError()) {
    return std::vector<Addon>();
  }

  std::vector<Addon> addons{};
  for (const auto* it = document.Begin(); it != document.End(); ++it) {
    Addon addon{};
    addon.type = addon_type;
    if (addon.Deserialize(it->GetObject())) {
      addons.push_back(addon);
    }
  }

  return addons;
}

}  // namespace addon_updater
