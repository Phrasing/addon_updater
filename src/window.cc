// clang-format off
#include "pch.h"
#include "window.h"

// clang-format on

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
                                                             UINT msg,
                                                             WPARAM wParam,
                                                             LPARAM lParam);
namespace addon_updater_window {

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
  using ret_cb = Ret (*)(Params...);
  template <typename... Args>
  static Ret callback(Args... args) {
    return function(args...);
  }

  static ret_cb GetCallback(std::function<Ret(Params...)> fn) {
    function = fn;
    return static_cast<ret_cb>(
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
    : d3d12_context_(std::make_shared<d3d12_context::D3D12Context>()) {
  WNDCLASSEXA window_class = {
      sizeof(WNDCLASSEXA),
      CS_CLASSDC,
      GET_CALLBACK(WNDPROC, Window)(std::bind(
          &Window::WndProc, this, std::placeholders::_1, std::placeholders::_2,
          std::placeholders::_3, std::placeholders::_4)),
      0L,
      0L,
      GetModuleHandle(nullptr),
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      window_title.data(),
      nullptr};

  RegisterClassExA(&window_class);

  window_handle_ = CreateWindowA(
      window_class.lpszClassName, window_title.data(), WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, window_size.first, window_size.second,
      nullptr, nullptr, window_class.hInstance, nullptr);

  if (!d3d12_context_->CreateDeviceD3D(window_handle_)) {
    d3d12_context_->CleanupDeviceD3D();
    ::UnregisterClassA(window_class.lpszClassName, window_class.hInstance);
    return;
  }

  ShowWindow(window_handle_, SW_SHOWDEFAULT);
  UpdateWindow(window_handle_);
}

Window::~Window() {}

void Window::Render(
    const std::function<void(const std::pair<int32_t, int32_t>& window_size)>
        draw_callback) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO& io = ImGui::GetIO();

  ImGui::StyleColorsDark();

  ImGui_ImplWin32_Init(window_handle_);
  ImGui_ImplDX12_Init(
      d3d12_context_->device, d3d12_context::kNumFramesInFlight,
      DXGI_FORMAT_R8G8B8A8_UNORM, d3d12_context_->srv_descriptor_heap,
      d3d12_context_->srv_descriptor_heap->GetCPUDescriptorHandleForHeapStart(),
      d3d12_context_->srv_descriptor_heap
          ->GetGPUDescriptorHandleForHeapStart());

  io.IniFilename = nullptr;

  ImFontConfig config;
  config.OversampleH = 3;
  config.OversampleV = 3;

  io.Fonts->AddFontDefault(&config);

  uint32_t flags = ImGuiFreeType::NoHinting;
  ImGuiFreeType::BuildFontAtlas(io.Fonts, flags);

  MSG msg;
  ZeroMemory(&msg, sizeof(msg));
  while (msg.message != WM_QUIT) {
    if (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
      ::TranslateMessage(&msg);
      ::DispatchMessage(&msg);
      continue;
    }

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    draw_callback(window_size_);

    d3d12_context::FrameContext* frame_ctx =
        d3d12_context_->WaitForNextFrameResources();
    auto back_buffer_index =
        d3d12_context_->swap_chain->GetCurrentBackBufferIndex();
    frame_ctx->CommandAllocator->Reset();

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource =
        d3d12_context_->main_render_target_resource[back_buffer_index];
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

    d3d12_context_->command_list->Reset(frame_ctx->CommandAllocator, nullptr);
    d3d12_context_->command_list->ResourceBarrier(1, &barrier);
    d3d12_context_->command_list->ClearRenderTargetView(
        d3d12_context_->main_render_target_descriptor[back_buffer_index],
        (float*)&ImVec4(0.1f, 0.1f, 0.1f, 0.1f), 0, nullptr);
    d3d12_context_->command_list->OMSetRenderTargets(
        1, &d3d12_context_->main_render_target_descriptor[back_buffer_index],
        FALSE, nullptr);
    d3d12_context_->command_list->SetDescriptorHeaps(
        1, &d3d12_context_->srv_descriptor_heap);

    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(),
                                  d3d12_context_->command_list);

    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    d3d12_context_->command_list->ResourceBarrier(1, &barrier);
    d3d12_context_->command_list->Close();

    d3d12_context_->command_queue->ExecuteCommandLists(
        1, reinterpret_cast<ID3D12CommandList* const*>(
               &d3d12_context_->command_list));

    d3d12_context_->swap_chain->Present(1, 0);  // Present with vsync

    const auto fence_value = d3d12_context_->fence_last_signaled_value + 1;
    d3d12_context_->command_queue->Signal(d3d12_context_->fence, fence_value);
    d3d12_context_->fence_last_signaled_value = fence_value;
    frame_ctx->FenceValue = fence_value;
  }

  d3d12_context_->WaitForLastSubmittedFrame();
  ImGui_ImplDX12_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();

  d3d12_context_->CleanupDeviceD3D();
  ::DestroyWindow(window_handle_);
}

LRESULT __stdcall Window::WndProc(HWND hWnd, UINT msg, WPARAM wParam,
                                  LPARAM lParam) {
  if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;

  switch (msg) {
    case WM_SIZE:
      if (d3d12_context_->device != nullptr && wParam != SIZE_MINIMIZED) {
        d3d12_context_->WaitForLastSubmittedFrame();
        ImGui_ImplDX12_InvalidateDeviceObjects();
        d3d12_context_->CleanupRenderTarget();
        d3d12_context_->ResizeSwapChain(hWnd, (UINT)LOWORD(lParam),
                                        (UINT)HIWORD(lParam));
        d3d12_context_->CreateRenderTarget();
        ImGui_ImplDX12_CreateDeviceObjects();

        window_size_ = std::make_pair(LOWORD(lParam), HIWORD(lParam));
      }
      return 0;
    case WM_SYSCOMMAND:
      if ((wParam & 0xfff0) == SC_KEYMENU)  // Disable ALT application menu
        return 0;
      break;
    case WM_DESTROY:
      ::PostQuitMessage(0);
      return 0;
  }
  return ::DefWindowProc(hWnd, msg, wParam, lParam);
}  // namespace addon_updater_window

namespace d3d12_context {
bool D3D12Context::CreateDeviceD3D(HWND hWnd) {
  // Setup swap chain
  DXGI_SWAP_CHAIN_DESC1 swap_description;
  {
    ZeroMemory(&swap_description, sizeof(swap_description));
    swap_description.BufferCount = kNumOfBackBuffers;
    swap_description.Width = 0;
    swap_description.Height = 0;
    swap_description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_description.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
    swap_description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_description.SampleDesc.Count = 1;
    swap_description.SampleDesc.Quality = 0;
    swap_description.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swap_description.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swap_description.Scaling = DXGI_SCALING_STRETCH;
    swap_description.Stereo = FALSE;
  }

#ifdef DX12_ENABLE_DEBUG_LAYER
  ID3D12Debug* d3d12_debug = nullptr;
  if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&d3d12_debug)))) {
    d3d12_debug->EnableDebugLayer();
    d3d12_debug->Release();
  }
#endif

  D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
  if (D3D12CreateDevice(nullptr, featureLevel, IID_PPV_ARGS(&device)) != S_OK)
    return false;

  {
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    desc.NumDescriptors = kNumOfBackBuffers;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    desc.NodeMask = 1;
    if (device->CreateDescriptorHeap(
            &desc, IID_PPV_ARGS(&rtv_descriptor_heap)) != S_OK)
      return false;

    const auto rtv_descriptor_size = device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
        rtv_descriptor_heap->GetCPUDescriptorHandleForHeapStart();
    for (auto& i : main_render_target_descriptor) {
      i = rtvHandle;
      rtvHandle.ptr += rtv_descriptor_size;
    }
  }

  {
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = 1;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (device->CreateDescriptorHeap(
            &desc, IID_PPV_ARGS(&srv_descriptor_heap)) != S_OK)
      return false;
  }

  {
    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.NodeMask = 1;
    if (device->CreateCommandQueue(&desc, IID_PPV_ARGS(&command_queue)) != S_OK)
      return false;
  }

  for (auto& i : frame_context)
    if (device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                       IID_PPV_ARGS(&i.CommandAllocator)) !=
        S_OK)
      return false;

  if (device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                frame_context[0].CommandAllocator, nullptr,
                                IID_PPV_ARGS(&command_list)) != S_OK ||
      command_list->Close() != S_OK)
    return false;

  if (device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)) !=
      S_OK)
    return false;

  fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  if (fence_event == nullptr) return false;

  {
    IDXGIFactory4* dxgi_factory = nullptr;
    IDXGISwapChain1* dxgi_swap_chain = nullptr;
    if (CreateDXGIFactory1(IID_PPV_ARGS(&dxgi_factory)) != S_OK ||
        dxgi_factory->CreateSwapChainForHwnd(
            command_queue, hWnd, &swap_description, nullptr, nullptr,
            &dxgi_swap_chain) != S_OK ||
        dxgi_swap_chain->QueryInterface(IID_PPV_ARGS(&swap_chain)) != S_OK) {
      return false;
    }

    dxgi_swap_chain->Release();
    dxgi_factory->Release();
    swap_chain->SetMaximumFrameLatency(kNumOfBackBuffers);
    swap_chain_waitable_object = swap_chain->GetFrameLatencyWaitableObject();
  }

  CreateRenderTarget();
  return true;
}

void D3D12Context::CleanupDeviceD3D() {
  CleanupRenderTarget();
  if (swap_chain) {
    swap_chain->Release();
    swap_chain = nullptr;
  }
  if (swap_chain_waitable_object != nullptr) {
    CloseHandle(swap_chain_waitable_object);
  }
  for (auto& i : frame_context)
    if (i.CommandAllocator) {
      i.CommandAllocator->Release();
      i.CommandAllocator = nullptr;
    }
  if (command_queue) {
    command_queue->Release();
    command_queue = nullptr;
  }
  if (command_list) {
    command_list->Release();
    command_list = nullptr;
  }
  if (rtv_descriptor_heap) {
    rtv_descriptor_heap->Release();
    rtv_descriptor_heap = nullptr;
  }
  if (srv_descriptor_heap) {
    srv_descriptor_heap->Release();
    srv_descriptor_heap = nullptr;
  }
  if (fence) {
    fence->Release();
    fence = nullptr;
  }
  if (fence_event) {
    CloseHandle(fence_event);
    fence_event = nullptr;
  }
  if (device) {
    device->Release();
    device = nullptr;
  }

#ifdef DX12_ENABLE_DEBUG_LAYER
  IDXGIDebug1* dxgi_debug = nullptr;
  if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgi_debug)))) {
    dxgi_debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
    dxgi_debug->Release();
  }
#endif
}

void D3D12Context::CreateRenderTarget() {
  for (uint32_t i = 0; i < kNumOfBackBuffers; i++) {
    ID3D12Resource* d3d_back_buffer = nullptr;
    swap_chain->GetBuffer(i, IID_PPV_ARGS(&d3d_back_buffer));
    device->CreateRenderTargetView(d3d_back_buffer, nullptr,
                                   main_render_target_descriptor[i]);
    main_render_target_resource[i] = d3d_back_buffer;
  }
}

void D3D12Context::CleanupRenderTarget() {
  WaitForLastSubmittedFrame();

  for (auto& i : main_render_target_resource)
    if (i) {
      i->Release();
      i = nullptr;
    }
}

void D3D12Context::WaitForLastSubmittedFrame() {
  FrameContext* frame_ctx = &frame_context[frame_index % kNumFramesInFlight];

  UINT64 fenceValue = frame_ctx->FenceValue;
  if (fenceValue == 0) return;  // No fence was signaled

  frame_ctx->FenceValue = 0;
  if (fence->GetCompletedValue() >= fenceValue) return;

  fence->SetEventOnCompletion(fenceValue, fence_event);
  WaitForSingleObject(fence_event, INFINITE);
}

FrameContext* D3D12Context::WaitForNextFrameResources() {
  const auto next_frame_index = frame_index + 1;
  frame_index = next_frame_index;

  HANDLE waitable_objects[] = {swap_chain_waitable_object, nullptr};
  DWORD waitable_objects_count = 1;

  auto* frame_ctx = &frame_context[next_frame_index % kNumFramesInFlight];
  const auto fence_value = frame_ctx->FenceValue;
  if (fence_value != 0)  // means no fence was signaled
  {
    frame_ctx->FenceValue = 0;
    fence->SetEventOnCompletion(fence_value, fence_event);
    waitable_objects[1] = fence_event;
    waitable_objects_count = 2;
  }

  WaitForMultipleObjects(waitable_objects_count, waitable_objects, TRUE,
                         INFINITE);

  return frame_ctx;
}

void D3D12Context::ResizeSwapChain(HWND hWnd, int width, int height) {
  DXGI_SWAP_CHAIN_DESC1 swap_description;
  swap_chain->GetDesc1(&swap_description);
  swap_description.Width = width;
  swap_description.Height = height;

  IDXGIFactory4* dxgi_factory = nullptr;
  swap_chain->GetParent(IID_PPV_ARGS(&dxgi_factory));

  swap_chain->Release();
  CloseHandle(swap_chain_waitable_object);

  IDXGISwapChain1* dxgi_swap_chain = nullptr;
  dxgi_factory->CreateSwapChainForHwnd(command_queue, hWnd, &swap_description,
                                       nullptr, nullptr, &dxgi_swap_chain);
  dxgi_swap_chain->QueryInterface(IID_PPV_ARGS(&swap_chain));
  dxgi_swap_chain->Release();
  dxgi_factory->Release();

  swap_chain->SetMaximumFrameLatency(kNumOfBackBuffers);

  swap_chain_waitable_object = swap_chain->GetFrameLatencyWaitableObject();
  assert(swap_chain_waitable_object != nullptr);
}

}  // namespace d3d12_context

}  // namespace addon_updater_window
