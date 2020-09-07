#ifndef ADDON_UPDATER_WINDOW_H
#define ADDON_UPDATER_WINDOW_H
#pragma once

namespace addon_updater_window {
class Window {
 public:
  Window(std::string_view window_title, std::pair<int32_t, int32_t> window_size);
  ~Window();



 private:
};
}  // namespace addon_updater_window

#endif  // !ADDON_UPDATER_WINDOW_H
