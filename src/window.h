#ifndef ADDON_UPDATER_WINDOW_H
#define ADDON_UPDATER_WINDOW_H
#pragma once

namespace addon_updater_window {
namespace d3d12_context {

constexpr auto kNumFramesInFlight = 3;
constexpr auto kNumOfBackBuffers = 3;

struct FrameContext {
  ID3D12CommandAllocator* CommandAllocator;
  UINT64 FenceValue;
};

struct D3D12Context {
  FrameContext frame_context[kNumFramesInFlight] = {};
  UINT frame_index = 0;
  ID3D12Device* device = nullptr;
  ID3D12DescriptorHeap* rtv_descriptor_heap = nullptr;
  ID3D12DescriptorHeap* srv_descriptor_heap = nullptr;
  ID3D12CommandQueue* command_queue = nullptr;
  ID3D12GraphicsCommandList* command_list = nullptr;
  ID3D12Fence* fence = nullptr;
  HANDLE fence_event = nullptr;
  UINT64 fence_last_signaled_value = 0;
  IDXGISwapChain3* swap_chain = nullptr;
  HANDLE swap_chain_waitable_object = nullptr;
  ID3D12Resource* main_render_target_resource[kNumOfBackBuffers] = {};
  D3D12_CPU_DESCRIPTOR_HANDLE
  main_render_target_descriptor[kNumOfBackBuffers] = {};

  bool CreateDeviceD3D(HWND hWnd);
  void CleanupDeviceD3D();
  void CreateRenderTarget();
  void CleanupRenderTarget();
  void WaitForLastSubmittedFrame();
  FrameContext* WaitForNextFrameResources();
  void ResizeSwapChain(HWND hWnd, int width, int height);
};
}  // namespace d3d12_context

class Window {
 public:
  Window(std::string_view window_title,
         std::pair<int32_t, int32_t> window_size);
  ~Window();

  void Render(
      const std::function<void(const std::pair<int32_t, int32_t>& window_size)>
          draw_callback);

 private:
  LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
  HWND window_handle_;
  WNDCLASSEXA window_class_;
  std::pair<int32_t, int32_t> window_size_;
  std::shared_ptr<d3d12_context::D3D12Context> d3d12_context_;
};
}  // namespace addon_updater_window

#endif  // !ADDON_UPDATER_WINDOW_H
