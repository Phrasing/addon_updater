// clang-format off
#include "pch.h"
#include "window.h"
#include "resource_loader.h"
#include "windows_error_message.h"
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
  glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

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

  glewInit();

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO& io = ImGui::GetIO();
  io.IniFilename = nullptr;

  if (const auto default_font = addon_updater::GetResource(
          DEFAULT_FONT_RESOURCE, addon_updater::ResourceType::kBinary);
      default_font.has_value()) {
    ImFontConfig config{};
    config.FontDataOwnedByAtlas = false;
    io.Fonts->AddFontFromMemoryTTF(default_font->data, default_font->size, 20.F,
                                   &config);
  }

  if (const auto font_awesome = addon_updater::GetResource(
          FONT_AWESOME_RESOURCE, addon_updater::ResourceType::kBinary);
      font_awesome.has_value()) {
    ImFontConfig config{};
    config.MergeMode = true;
    config.FontDataOwnedByAtlas = false;
    static const ImWchar icon_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
    io.Fonts->AddFontFromMemoryTTF(font_awesome->data, font_awesome->size, 17.F,
                                   &config, icon_ranges);
  }

  unsigned int flags = ImGuiFreeType::RasterizerFlags::NoHinting;
  if (!ImGuiFreeType::BuildFontAtlas(io.Fonts, flags)) {
    addon_updater::WindowsErrorMessageBox("Error: failed to build font atlas.");
  }
  auto& style = ImGui::GetStyle();
  style.ChildRounding = 3.f;
  style.GrabRounding = 0.f;
  style.WindowRounding = 0.f;
  style.ScrollbarRounding = 3.f;
  style.FrameRounding = 3.f;
  style.WindowTitleAlign = ImVec2(0.5f, 0.5f);

  ImVec4* colors = ImGui::GetStyle().Colors;
  colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
  colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
  colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
  colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
  colors[ImGuiCol_Border] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
  colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  colors[ImGuiCol_FrameBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
  colors[ImGuiCol_TitleBg] = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);
  
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);
  colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
  colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.13f, 0.13f, 0.35f);
  colors[ImGuiCol_ScrollbarBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.53f);
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
  colors[ImGuiCol_CheckMark] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
  colors[ImGuiCol_SliderGrab] = ImVec4(0.16f, 0.17f, 0.17f, 1.00f);
  colors[ImGuiCol_SliderGrabActive] = ImVec4(0.42f, 0.42f, 0.42f, 1.00f);
  colors[ImGuiCol_Button] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
  colors[ImGuiCol_ButtonHovered] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
  colors[ImGuiCol_ButtonActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
  colors[ImGuiCol_Header] = ImVec4(0.16f, 0.16f, 0.16f, 0.31f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
  colors[ImGuiCol_HeaderActive] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
  colors[ImGuiCol_Separator] = ImVec4(0.34f, 0.34f, 0.34f, 1.00f);
  colors[ImGuiCol_SeparatorHovered] = ImVec4(0.42f, 0.42f, 0.42f, 1.00f);
  colors[ImGuiCol_SeparatorActive] = ImVec4(0.81f, 0.81f, 0.81f, 1.00f);
  colors[ImGuiCol_ResizeGrip] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
  colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.17f, 0.17f, 0.18f, 1.00f);
  colors[ImGuiCol_ResizeGripActive] = ImVec4(0.23f, 0.23f, 0.23f, 1.00f);
  colors[ImGuiCol_Tab] = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
  colors[ImGuiCol_TabHovered] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
  colors[ImGuiCol_TabActive] = ImVec4(0.44f, 0.45f, 0.45f, 0.49f);
  colors[ImGuiCol_TabUnfocused] = ImVec4(0.19f, 0.19f, 0.19f, 0.97f);
  colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.37f, 0.37f, 0.37f, 1.00f);
  colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
  colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
  colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
  colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
  colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
  colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
  colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
  colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
  colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
  colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

  ImGui_ImplGlfw_InitForOpenGL(glfw_ctx_->window, true);
  ImGui_ImplOpenGL3_Init();

  glfw_ctx_->clear_color = ImVec4(0.10F, 0.10F, 0.10F, 1.00F);
}

void Window::Render(const RenderCallback& draw_callback) {
  while (!glfwWindowShouldClose(glfw_ctx_->window)) {
    glfwWaitEvents();

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
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

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
