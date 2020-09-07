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

bool Addon::Install() { return false; }

std::string InstalledAddon::Serialize() const { return std::string(); }

void InstalledAddon::Uninstall() {
  for (auto& directory : directories) {
    if (!std::filesystem::remove_all(directory)) {
      std::cerr << "Failed to remove directory " << directory << std::endl;
    }
  }
}

bool InstalledAddon::Update() { return false; }

}  // namespace addon_updater
