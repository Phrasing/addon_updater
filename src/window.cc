// clang-format off
#include "pch.h"
#include "window.h"
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

template <typename T, unsigned int n, typename CallerType>
struct Callback;

template <typename Ret, typename... Params, unsigned int n, typename CallerType>
struct Callback<Ret(Params...), n, CallerType> {
  using InvokeCallback = Ret (*)(Params...);
  template <typename... Args>
  static Ret callback(Args... args) {
    return function(args...);
  }

  static InvokeCallback GetCallback(std::function<Ret(Params...)> fn) {
    function = fn;
    return static_cast<InvokeCallback>(
        Callback<Ret(Params...), n, CallerType>::callback);
  }

  static std::function<Ret(Params...)> function;
};

template <typename Ret, typename... Params, unsigned int n, typename CallerType>
std::function<Ret(Params...)> Callback<Ret(Params...), n, CallerType>::function;

#define GET_CALLBACK(ptr_type, caller_type) \
  Callback<ActualType<ptr_type>::type, __COUNTER__, caller_type>::GetCallback

};  // namespace

Window::Window(std::string_view window_title,
               std::pair<int32_t, int32_t> window_size)
    : glfw_ctx_(std::make_shared<GlfwContext>()) {
  glfwSetErrorCallback(GET_CALLBACK(GLFWerrorfun, Window)(
      std::bind(&Window::GlfwErrorCallback, this, std::placeholders::_1,
                std::placeholders::_2)));

  glfwInit();

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

  glfw_ctx_->window = glfwCreateWindow(window_size.first, window_size.second,
                                       window_title.data(), nullptr, nullptr);

  glfwMakeContextCurrent(glfw_ctx_->window);
  glfwSwapInterval(1);  // Enable vsync

  static_cast<void>(glewInit());

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO& io = ImGui::GetIO();
  io.IniFilename = nullptr;

  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForOpenGL(glfw_ctx_->window, true);
  ImGui_ImplOpenGL3_Init("#version 130");

  glfw_ctx_->clear_color = ImVec4(0.45F, 0.55F, 0.60F, 1.00F);
}


void Window::Render(const RenderCallback& draw_callback) {
  while (!glfwWindowShouldClose(glfw_ctx_->window)) {
    glfwPollEvents();

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    { draw_callback(glfw_ctx_->window_size); }
    ImGui::Render();
    glfwGetFramebufferSize(glfw_ctx_->window, &glfw_ctx_->window_size.first,
                           &glfw_ctx_->window_size.second);
    glViewport(0, 0, glfw_ctx_->window_size.first,
               glfw_ctx_->window_size.second);
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

void Window::GlfwErrorCallback(int error_code, const char* description) {
  std::cerr << error_code << " " << description << std::endl;
}

}  // namespace addon_updater_window
