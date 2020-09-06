#ifndef ADDON_H
#define ADDON_H
#pragma once

namespace addon_updater {

enum class AddonType { kTukui, kCurse };

struct Addon {
  Addon(AddonType addon_type) : addon_type(addon_type){};

  bool Deserialize();
  bool Serialize();

  std::string screenshot_url;
  std::string download_url;
  AddonType addon_type;
};
}  // namespace addon_updater

#endif  // !ADDON_H
