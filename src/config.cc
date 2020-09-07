// clang-format off
#include "pch.h"
#include "config.h"
// clang-format on

std::string addon_updater::UpdaterConfig::Serialize() { return std::string(); }

bool addon_updater::UpdaterConfig::DeserializeFromFile(
    const std::string_view file_contents) {
  rj::Document document{};
  document.Parse(file_contents.data());

  if (document.HasParseError()) {
    return false;
  }

  InstalledAddon installed_addon{};
  for (const auto* it = document.Begin(); it != document.End(); ++it) {
    if (it->IsObject()) {
      if (installed_addon.Deserialize(it->GetObject())) {
        installed_addons_.push_back(installed_addon);
      }
    }
  }

  return true;
}

bool addon_updater::UpdaterConfig::Ingest() {
  auto config_file = std::make_unique<std::ifstream>(config_file_path_);

  if (!config_file->good()) return false;

  const auto contents =
      std::string{std::istreambuf_iterator<char>((*config_file)),
                  std::istreambuf_iterator<char>()};

  if (!DeserializeFromFile(contents)) return false;

  return true;
}

bool addon_updater::UpdaterConfig::UpdateFile() { return false; }

bool addon_updater::UpdaterConfig::UninstallAddon(
    const InstalledAddon& installed_addon) {
  const auto result =
      std::find_if(installed_addons_.begin(), installed_addons_.end(),
                   [&](const InstalledAddon& addon) {
                     return addon.id == installed_addon.id;
                   });

  if (result == std::end(installed_addons_)) return false;

  installed_addons_.erase(result);
  return false;
}

bool addon_updater::UpdaterConfig::InstallAddon() { return false; }
