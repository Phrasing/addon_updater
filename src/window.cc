// clang-format off
#include "pch.h"
#include "window.h"
#include "resource_loader.h"
#include "../data/resource/resource.h"

#include <windowsx.h>
// clang-format on

namespace addon_updater {

namespace {
template <typename T>
struct ActualType {
  using type = T;
};
template <typename T>
struct ActualType<T*> {
  using type = typename ActualType<T>::type;
};

template <typename T, uint32_t uid, typename CallerType>
struct Callback;

template <typename Ret, typename... Params, uint32_t uid, typename CallerType>
struct Callback<Ret(Params...), uid, CallerType> {
  using InvokeCallback = Ret (*)(Params...);
  template <typename... Args>
  static Ret callback(Args... args) {
    return Function(args...);
  }

  static InvokeCallback GetCallback(std::function<Ret(Params...)> fn) {
    Function = fn;
    return static_cast<InvokeCallback>(
        Callback<Ret(Params...), uid, CallerType>::callback);
  }

  static std::function<Ret(Params...)> Function;
};

template <typename Ret, typename... Params, uint32_t uid, typename CallerType>
std::function<Ret(Params...)>
    Callback<Ret(Params...), uid, CallerType>::Function;

#define GET_CALLBACK(ptr_type, caller_type) \
  Callback<ActualType<ptr_type>::type, __COUNTER__, caller_type>::GetCallback

};  // namespace

Window::Window(std::string_view window_title, const WindowSize& window_size)
    : glfw_ctx_(std::make_shared<GlfwContext>()) {
  glfwInit();
  
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_DECORATED, GL_FALSE);

  glfw_ctx_->window = glfwCreateWindow(window_size.width, window_size.height,
                                       window_title.data(), nullptr, nullptr);

  glfw_ctx_->native_window_handle = glfwGetWin32Window(glfw_ctx_->window);

  glfwSetErrorCallback(GET_CALLBACK(GLFWerrorfun, Window)(
      std::bind(&Window::GlfwErrorCallback, this, std::placeholders::_1,
                std::placeholders::_2)));

  glfw_ctx_->window_callback_ =
      SubclassWindow(glfw_ctx_->native_window_handle,
                     GET_CALLBACK(WNDPROC, Window)(std::bind(
                         &Window::WindowCallback, this, std::placeholders::_1,
                         std::placeholders::_2, std::placeholders::_3,
                         std::placeholders::_4)));

  glfwMakeContextCurrent(glfw_ctx_->window);
  glfwSwapInterval(1);

  static_cast<void>(glewInit());

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO& io = ImGui::GetIO();
  io.IniFilename = nullptr;

  if (auto default_font = addon_updater::GetResource(
          DEFAULT_FONT_RESOURCE, addon_updater::ResourceType::kBinary);
      default_font != std::nullopt) {
    ImFontConfig config{};
    config.FontDataOwnedByAtlas = true;
    io.Fonts->AddFontFromMemoryTTF(default_font->data, default_font->size, 20.F,
                                   &config, io.Fonts->GetGlyphRangesDefault());
  }

  if (auto font_awesome = addon_updater::GetResource(
          FONT_AWESOME_RESOURCE, addon_updater::ResourceType::kBinary);
      font_awesome != std::nullopt) {
    ImFontConfig config{};
    config.MergeMode = true;
    config.FontDataOwnedByAtlas = true;
    const ImWchar icon_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
    io.Fonts->AddFontFromMemoryTTF(font_awesome->data, font_awesome->size, 18.F,
                                   &config, icon_ranges);
  }

  unsigned int flags = ImGuiFreeType::RasterizerFlags::NoHinting;
  ImGuiFreeType::BuildFontAtlas(io.Fonts, flags);

  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForOpenGL(glfw_ctx_->window, true);
  ImGui_ImplOpenGL3_Init();

  auto& style = ImGui::GetStyle();
  style.WindowRounding = 0.F;
  style.FrameRounding = 0.F;

  glfw_ctx_->clear_color = ImVec4(0.45F, 0.55F, 0.60F, 1.00F);
}

void Window::Render(const RenderCallback& draw_callback) {
  while (!glfwWindowShouldClose(glfw_ctx_->window)) {
    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    draw_callback(glfw_ctx_->window_size);

    ImGui::Render();
    glfwGetFramebufferSize(glfw_ctx_->window, &glfw_ctx_->window_size.width,
                           &glfw_ctx_->window_size.height);

    glViewport(0, 0, glfw_ctx_->window_size.width,
               glfw_ctx_->window_size.height);

    glClearColor(glfw_ctx_->clear_color.x, glfw_ctx_->clear_color.y,
                 glfw_ctx_->clear_color.z, glfw_ctx_->clear_color.w);

    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(std::move(ImGui::GetDrawData()));

    glfwSwapBuffers(glfw_ctx_->window);
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(glfw_ctx_->window);
  glfwTerminate();
}

LRESULT Window::WindowCallback(HWND hwnd, UINT msg, WPARAM wparam,
                               LPARAM lparam) {
  auto IsOutsideDraggableRegion = [hwnd]() -> bool {
    POINT p;
    RECT r;

    GetCursorPos(&p);
    ScreenToClient(hwnd, &p);
    GetWindowRect(hwnd, &r);

    return p.y < 30 && p.x < (r.right - r.left) - 50;
  };

  if (msg == WM_NCHITTEST) {
    const auto hit =
        CallWindowProcW(glfw_ctx_->window_callback_, hwnd, msg, wparam, lparam);
    if (hit == HTCLIENT) {
      if (IsOutsideDraggableRegion()) return HTCAPTION;
    }
  }

  return CallWindowProc(glfw_ctx_->window_callback_, hwnd, msg, wparam, lparam);
}

void Window::GlfwErrorCallback(int error_code, const char* description) {
  std::fprintf(stderr, "GLFW Error: %i | %s\n", error_code, description);
}

}  // namespace addon_updater_window
