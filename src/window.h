#ifndef ADDON_UPDATER_WINDOW_H
#define ADDON_UPDATER_WINDOW_H
#pragma once

namespace addon_updater {

using RenderCallback = std::function<void(const std::pair<int32_t, int32_t>)>;

struct GlfwContext {
  GLFWwindow* window;
  HWND native_window_handle;
  ImVec4 clear_color;
  std::pair<int32_t, int32_t> window_size;
};

class Window {
 public:
  Window(std::string_view window_title,
         std::pair<int32_t, int32_t> window_size);
  ~Window() = default;

  void Render(const RenderCallback& draw_callback);

 private:
  void GlfwErrorCallback(int error_code, const char* description);
  std::shared_ptr<GlfwContext> glfw_ctx_;
};
}  // namespace addon_updater_window

#endif  // !ADDON_UPDATER_WINDOW_H
