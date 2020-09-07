#ifndef ADDON_UPDATER_CONFIG_H
#define ADDON_UPDATER_CONFIG_H
#pragma once

#include "addon.h"

namespace addon_updater {

struct UpdaterConfig {
  UpdaterConfig(const std::string_view config_file_path)
      : config_file_path_(config_file_path) {}

  std::string Serialize();
  bool DeserializeFromFile(std::string_view file_contents);

  bool Ingest();
  bool UpdateFile();

  bool UninstallAddon(const InstalledAddon& installed_addon);
  bool InstallAddon();

  std::optional<InstalledAddon> FindAddon(int32_t id);

  std::vector<InstalledAddon> installed_addons_;
  std::string config_file_path_;
};
}  // namespace addon_updater

#endif  // !ADDON_UPDATER_CONFIG_H
