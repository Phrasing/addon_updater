#ifndef ADDON_UPDATER_GUI_H
#define ADDON_UPDATER_GUI_H
#pragma once

#include "addon.h"
#include "window.h"

namespace addon_updater {
class Gui {
 public:
  Gui( boost::asio::thread_pool& thd_pool);
  ~Gui() = default;

  Gui(const Gui&) = delete;
  Gui& operator=(const Gui&) = delete;

  void DrawGui(Addons& addons, std::vector<InstalledAddon>& installed_addons,
               const WindowSize& window_size, bool is_loading);

 private:
  boost::asio::thread_pool* thd_pool_;

 private:
  void RenderBrowseTab(std::vector<addon_updater::Addon>& addons);
  void RenderInstalledTab(std::vector<addon_updater::InstalledAddon>& addons);
};
}  // namespace addon_updater_gui

#endif  // !ADDON_UPDATER_GUI_H
