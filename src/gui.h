#ifndef ADDON_UPDATER_GUI_H
#define ADDON_UPDATER_GUI_H
#pragma once

#include "addon.h"
#include "products.h"
#include "window.h"

namespace addon_updater {
class Gui {
 public:
  Gui(boost::asio::thread_pool& thd_pool,
      const WowInstallations& installations);
  ~Gui() = default;

  Gui(const Gui&) = delete;
  Gui& operator=(const Gui&) = delete;

  void DrawGui(Addons& addons, std::vector<InstalledAddon>& installed_addons,
               const WindowSize& window_size, bool is_loading);

 private:
  boost::asio::thread_pool* thd_pool_;

 private:
  void RenderBrowseTab(
      std::vector<addon_updater::Addon>& addons,
      std::vector<addon_updater::InstalledAddon>& installed_addons);
  void RenderInstalledTab(std::vector<addon_updater::InstalledAddon>& addons);

  void AsyncLoadAddonThumbnail(AddonThumbnail* thumbnail,
                               std::string_view screenshot_url);
  bool LoadAndResizeThumbnail(uint8_t* data, size_t data_size,
                              addon_updater::AddonThumbnail* thumbnail);

  std::string search_text_;

  WowInstall selected_installation_;
  WowInstallations installations_;

  uint8_t* curse_icon_ = nullptr;
  size_t curse_icon_size_ = 0u;
};
}  // namespace addon_updater_gui

#endif  // !ADDON_UPDATER_GUI_H
