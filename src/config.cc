// clang-format off
#include "pch.h"
#include "config.h"
#include "file.h"
// clang-format on

namespace addon_updater {

bool UpdaterConfig::SerializeToFile() {
  rj::StringBuffer string_buffer;
  rj::PrettyWriter<rj::StringBuffer> writer(string_buffer);
  writer.StartArray();
  {
    for (const auto& addon : this->installed_addons_) {
      addon.Serialize(&writer);
    }
  }
  writer.EndArray();

  auto config_file = std::make_unique<std::fstream>(
      this->config_file_path_, std::ios::out | std::ios::trunc);

  if (!config_file->is_open()) return false;

  config_file->write(string_buffer.GetString(), string_buffer.GetLength());

  return true;
}

bool UpdaterConfig::DeserializeFromFile() {
  auto file_result = addon_updater::ReadFile(this->config_file_path_.c_str());

  if (!file_result.Ok()) return false;

  rj::Document document{};
  document.Parse(file_result.content);

  if (document.HasParseError()) {
    return false;
  }

  InstalledAddon installed_addon{};
  for (const auto* it = document.Begin(); it != document.End(); ++it) {
    if (it->IsObject()) {
      if (installed_addon.Deserialize(it->GetObject())) {
        this->installed_addons_.push_back(installed_addon);
      }
    }
  }

  return true;
}

bool UpdaterConfig::UpdateConfig() { return false; }

bool UpdaterConfig::UninstallAddon(const InstalledAddon& installed_addon) {
  const auto result = FindAddon(installed_addon.id);

  if (result == std::nullopt) return false;

  installed_addons_.erase(std::remove(installed_addons_.begin(),
                                      installed_addons_.end(), result.value()),
                          installed_addons_.end());
  return false;
}

bool UpdaterConfig::InstallAddon(const Addon& addon) {
  installed_addons_.emplace_back(std::move(addon.Install()));
  return this->UpdateConfig();
}

std::optional<InstalledAddon> UpdaterConfig::FindAddon(int32_t id) {
  const auto result =
      std::find_if(installed_addons_.begin(), installed_addons_.end(),
                   [&](const InstalledAddon& addon) { return addon.id == id; });

  if (result == std::end(installed_addons_)) return std::nullopt;

  return (*result);
}

}  // namespace addon_updater
