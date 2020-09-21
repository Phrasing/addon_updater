#ifndef ADDON_UPDATER_WINDOW_H
#define ADDON_UPDATER_WINDOW_H
#pragma once

namespace addon_updater {

struct WindowSize {
  int width;
  int height;
};

struct GlfwContext {
  GLFWwindow* window;
  HWND native_window_handle;
  ImVec4 clear_color;
  WindowSize window_size;
  WNDPROC window_callback_;
};

using RenderCallback =
    std::function<void(const addon_updater::WindowSize& window_size)>;

class Window {
 public:
  Window(std::string_view window_title, const WindowSize& window_size);
  ~Window() = default;

  Window(const Window&) = delete;
  Window& operator=(const Window&) = delete;

  void Render(const RenderCallback& draw_callback);

 private:
  LRESULT WindowCallback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
  void GlfwErrorCallback(int error_code, const char* description);
  std::shared_ptr<GlfwContext> glfw_ctx_;
};
}  // namespace addon_updater_window

#endif  // !ADDON_UPDATER_WINDOW_H
