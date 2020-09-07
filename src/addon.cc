// clang-format off
#include "pch.h"
#include "addon.h"
// clang-format on

namespace addon_updater {

namespace {

bool DeserializeCurse(const rj::Value::ConstObject& object, Addon* addon) {
  MessageBoxA(nullptr, addon->hash.c_str(), nullptr, 0);
  return true;
}

bool DeserializeTukui(const rj::Value::ConstObject& object, Addon* addon) {
  return true;
}

}  // namespace

bool Addon::Deserialize(const rj::Value::ConstObject& object) {
  switch (type) {
    case AddonType::kCurse: {
      DeserializeCurse(object, this);
    } break;

    case AddonType::kTukui: {
      DeserializeTukui(object, this);
    } break;

    default:
      break;
  }
  return false;
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
  writer->Int(std::move(this->id));
  writer->String("name");
  writer->String(std::move(this->name));
  writer->String("downloadUrl");
  writer->String(std::move(this->download_url));
  writer->String("slug");
  writer->String(std::move(this->slug));
  writer->String("checksum");
  writer->String(std::move(this->hash));
  writer->String("flavor");
  writer->Int(static_cast<int32_t>(std::move(this->flavor)));
  writer->String("type");
  writer->Int(static_cast<int32_t>(std::move(this->type)));
  writer->String("version");
  writer->String(std::move(this->readable_version));

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

}  // namespace addon_updater
