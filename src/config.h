#ifndef ADDON_UPDATER_CONFIG_H
#define ADDON_UPDATER_CONFIG_H
#pragma once

#include "addon.h"

namespace addon_updater {

struct UpdaterConfig {
  explicit UpdaterConfig(std::string_view config_file_path)
      : config_file_path_(config_file_path) {}

  bool SerializeToFile();

  bool DeserializeFromFile();
  bool UpdateConfig();

  bool UninstallAddon(const InstalledAddon& installed_addon);
  bool InstallAddon(const Addon& addon);

  std::optional<InstalledAddon> FindAddon(int32_t id);

  std::vector<InstalledAddon> installed_addons_;
  std::string config_file_path_;
};
}  // namespace addon_updater

#endif  // !ADDON_UPDATER_CONFIG_H
