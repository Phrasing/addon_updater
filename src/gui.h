#ifndef ADDON_UPDATER_GUI_H
#define ADDON_UPDATER_GUI_H
#pragma once

#include "addon.h"
#include "window.h"

namespace addon_updater {
class Gui {
 public:
  Gui(boost::asio::thread_pool* thd_pool);
  ~Gui() = default;

  void DrawGui(std::vector<addon_updater::Addon>& addons,
               const std::pair<int32_t, int32_t>& window_size);

 private:
  boost::asio::thread_pool* thd_pool_;

 private:
  void RenderBrowseTab(std::vector<addon_updater::Addon>& addons);
};
}  // namespace addon_updater_gui

#endif  // !ADDON_UPDATER_GUI_H
