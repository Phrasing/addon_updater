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
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

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

  if (auto resource = addon_updater::GetResource(
          ARCHIVO_NARROW_RESOURCE, addon_updater::ResourceType::kBinary);
      resource != std::nullopt) {
    ImFontConfig config{};
    config.FontDataOwnedByAtlas = false;
    io.Fonts->AddFontFromMemoryTTF(resource->data, resource->size, 20.F,
                                   &config);
  }

  unsigned int flags = ImGuiFreeType::RasterizerFlags::NoHinting;
  ImGuiFreeType::BuildFontAtlas(io.Fonts, flags);

  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForOpenGL(glfw_ctx_->window, true);
  ImGui_ImplOpenGL3_Init();


  ImGuiStyle& style = ImGui::GetStyle();
  style.Colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
  style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
  style.Colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
  style.Colors[ImGuiCol_ChildBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
  style.Colors[ImGuiCol_PopupBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
  style.Colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
  style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  style.Colors[ImGuiCol_FrameBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
  style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
  style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.67f, 0.67f, 0.67f, 0.39f);
  style.Colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
  style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
  style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
  style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
  style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
  style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
  style.Colors[ImGuiCol_ScrollbarGrabHovered] =
      ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
  style.Colors[ImGuiCol_ScrollbarGrabActive] =
      ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
  style.Colors[ImGuiCol_CheckMark] = ImVec4(0.11f, 0.64f, 0.92f, 1.00f);
  style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.11f, 0.64f, 0.92f, 1.00f);
  style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.08f, 0.50f, 0.72f, 1.00f);
  style.Colors[ImGuiCol_Button] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
  style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
  style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.67f, 0.67f, 0.67f, 0.39f);
  style.Colors[ImGuiCol_Header] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
  style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
  style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.67f, 0.67f, 0.67f, 0.39f);
  style.Colors[ImGuiCol_Separator] = style.Colors[ImGuiCol_Border];
  style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.41f, 0.42f, 0.44f, 1.00f);
  style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
  style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.29f, 0.30f, 0.31f, 0.67f);
  style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
  style.Colors[ImGuiCol_Tab] = ImVec4(0.08f, 0.08f, 0.09f, 0.83f);
  style.Colors[ImGuiCol_TabHovered] = ImVec4(0.33f, 0.34f, 0.36f, 0.83f);
  style.Colors[ImGuiCol_TabActive] = ImVec4(0.23f, 0.23f, 0.24f, 1.00f);
  style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
  style.Colors[ImGuiCol_TabUnfocusedActive] =
      ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
  style.Colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
  style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
  style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
  style.Colors[ImGuiCol_PlotHistogramHovered] =
      ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
  style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
  style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.11f, 0.64f, 0.92f, 1.00f);
  style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
  style.Colors[ImGuiCol_NavWindowingHighlight] =
      ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
  style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
  style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
  style.GrabRounding = style.FrameRounding = 2.3f;

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
