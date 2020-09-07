// clang-format off
#include "pch.h"
#include "config.h"
// clang-format on

namespace addon_updater {

std::string UpdaterConfig::Serialize() { return std::string(); }

bool UpdaterConfig::DeserializeFromFile() {
  auto config_file = std::make_unique<std::ifstream>(this->config_file_path_);

  if (!config_file->good()) return false;

  const auto contents =
      std::string{std::istreambuf_iterator<char>((*config_file)),
                  std::istreambuf_iterator<char>()};

  rj::Document document{};
  document.Parse(contents.data());

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

bool UpdaterConfig::UpdateFile() { return false; }

bool UpdaterConfig::UninstallAddon(const InstalledAddon& installed_addon) {
  const auto result = FindAddon(installed_addon.id);

  if (result == std::nullopt) return false;

  installed_addons_.erase(std::remove(installed_addons_.begin(),
                                      installed_addons_.end(), result.value()),
                          installed_addons_.end());
  return false;
}

bool UpdaterConfig::InstallAddon() { return false; }

std::optional<InstalledAddon> UpdaterConfig::FindAddon(int32_t id) {
  const auto result =
      std::find_if(installed_addons_.begin(), installed_addons_.end(),
                   [&](const InstalledAddon& addon) { return addon.id == id; });

  if (result == std::end(installed_addons_)) return std::nullopt;

  return (*result);
}

}  // namespace addon_updater
