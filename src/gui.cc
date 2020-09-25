// clang-format off
#include "pch.h"
#include "gui.h"
#include "texture.h"
#include "file.h"
#include "http_client.h"
// clang-format on

namespace ImGui {
bool BufferingBar(const char* label, float value, const ImVec2& size_arg) {
  auto* window = GetCurrentWindow();
  if (window->SkipItems) return false;

  const ImU32 fg_col = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
  const ImU32 bg = ImGui::GetColorU32(ImGuiCol_Button);

  auto& ctx = *GImGui;
  const ImGuiStyle& style = ctx.Style;
  const ImGuiID id = window->GetID(label);
  const ImVec2 pos = window->DC.CursorPos;

  ImVec2 size = size_arg;
  size.x -= style.FramePadding.x * 2;

  const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
  ItemSize(bb, style.FramePadding.y);
  if (!ItemAdd(bb, id)) return false;

  const auto start = size.x * 0.7f;
  const auto end = size.x;
  const auto width = end - start;

  window->DrawList->AddRectFilled(bb.Min, ImVec2(pos.x + start, bb.Max.y), bg);
  window->DrawList->AddRectFilled(
      bb.Min, ImVec2(pos.x + start * value, bb.Max.y), fg_col);

  return true;
}

bool Spinner(const char* label, const ImVec2& radius, int thickness) {
  auto* window = ImGui::GetCurrentWindow();
  if (window->SkipItems) return false;

  auto& ctx = *GImGui;
  const auto& style = ctx.Style;
  const auto id = window->GetID(label);

  const auto pos = window->DC.CursorPos;
  const auto size =
      ImVec2((radius.x) * 2, (radius.y + style.FramePadding.y) * 2);

  const auto bb = ImRect(pos, ImVec2(pos.x + size.x, pos.y + size.y));
  ItemSize(bb, style.FramePadding.y);
  if (!ItemAdd(bb, id)) return false;

  window->DrawList->PathClear();

  constexpr auto num_segments = 30;

  const auto start = std::abs(ImSin(ctx.Time * 1.8f) * (num_segments - 5));

  const float a_min = IM_PI * 2.0F * (static_cast<float>(start)) /
                      static_cast<float>(num_segments);
  const float a_max = IM_PI * 2.0F * (static_cast<float>(num_segments) - 3) /
                      static_cast<float>(num_segments);

  const ImVec2 centre =
      ImVec2(pos.x + radius.x, pos.y + radius.y + style.FramePadding.y);

  for (auto i = 0; i < num_segments; i++) {
    const float a =
        a_min + (static_cast<float>(i) / static_cast<float>(num_segments)) *
                    (a_max - a_min);
    window->DrawList->PathLineTo(
        ImVec2(centre.x + ImCos(a + ctx.Time * 8) * radius.x,
               centre.y + ImSin(a + ctx.Time * 8) * radius.y));
  }

  window->DrawList->PathStroke(ImGui::GetColorU32(ImGuiCol_ButtonHovered),
                               false, thickness);
  return true;
}
}

namespace addon_updater {
namespace {

constexpr auto kThumbnailWidth = 32;
constexpr auto kThumbnailHeight = 32;

void HandleAddonDownload(const beast::error_code& ec,
                         std::string_view addon_data) {}

void RenderAddonDownloadStatus(const DownloadStatus& download_status) {}

void RenderLoadingScreen(const WindowSize& window_size) {
  ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
  ImGui::SetNextWindowSize(ImVec2(window_size.width, window_size.height));
  ImGui::Begin("##loading_screen", nullptr,
               ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoTitleBar);
  {
    const auto spinner_size = ImVec2(100, 100);
    ImGui::SetCursorPos((ImGui::GetWindowSize() - spinner_size) * 0.35F);
    ImGui::Spinner("##loading_spinner", spinner_size, 25);
  }
  ImGui::End();
}

}  // namespace

Gui::Gui(boost::asio::thread_pool& thd_pool) : thd_pool_(&thd_pool) {}

void Gui::DrawGui(Addons& addons, std::vector<InstalledAddon>& installed_addons,
                  const WindowSize& window_size, bool is_loading) {
  ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
  ImGui::SetNextWindowSize(ImVec2(window_size.width, window_size.height));

  if (is_loading) {
    RenderLoadingScreen(window_size);
    return;
  }

  ImGui::Begin("##MAIN", nullptr,
               ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
  {
    if (ImGui::BeginTabBar("##MAIN_TAB_BAR")) {
      if (ImGui::BeginTabItem("Browse  " ICON_FA_SEARCH)) {
        this->RenderBrowseTab(addons);
        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Installed")) {
        this->RenderInstalledTab(installed_addons);
        ImGui::EndTabItem();
      }
    }
    ImGui::EndTabBar();
    ImGui::End();
  }
}

void Gui::RenderBrowseTab(std::vector<addon_updater::Addon>& addons) {
  for (auto& addon : addons) {
    if (addon.thumbnail.is_uploaded) {
      ImGui::Image(addon.thumbnail.pixels,
                   ImVec2(addon.thumbnail.width, addon.thumbnail.height));
      ImGui::SameLine();
    }

    ImGui::Text(addon.name);

    for (auto& latest : addon.latest_file.modules) {
      ImGui::Text(latest.folder_name + " | " +
                  std::to_string(latest.finger_print));
    }

    if (addon.download_status.state != RequestState::kStatePending &&
        addon.download_status.state != RequestState::kStateFinish) {
      ImGui::SameLine();
      if (ImGui::Button((std::string(ICON_FA_DOWNLOAD "##") +
                         addon.remote_version.readable_version)
                            .c_str())) {
        addon.download_status.state = RequestState::kStatePending;
        ClientFactory::GetInstance().NewAsyncClient()->Download(
            addon.download_url,
            [&](const beast::error_code& ec, std::string_view response) {

            },
            [&](const DownloadStatus& download_status) {
              if (addon.download_status.state == RequestState::kStateCancel) {
                return false;
              }
              addon.download_status = download_status;
              return true;
            });
      }
    }

    if (addon.download_status.state == RequestState::kStatePending) {
      ImGui::SameLine();
      ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8.0F);
      ImGui::BufferingBar("##buffer_bar",
                          addon.download_status.progress / 100.F,
                          ImVec2(200, 6));
      ImGui::SameLine();
      if (ImGui::Button((std::string(ICON_FA_STOP_CIRCLE "##") +
                         addon.remote_version.readable_version)
                            .c_str())) {
        addon.download_status.state = RequestState::kStateCancel;
      }
    }

    if (addon.thumbnail.is_loaded && !addon.thumbnail.is_uploaded) {
      auto* uploaded_texture =
          UploadTexture(addon.thumbnail.pixels, addon.thumbnail.width,
                        addon.thumbnail.height, addon.thumbnail.channels);

      if (uploaded_texture != nullptr) {
        addon.thumbnail.pixels = static_cast<uint8_t*>(uploaded_texture);
      }

      addon.thumbnail.is_uploaded = true;
    }

    if (ImGui::IsItemVisible() && !addon.thumbnail.is_loaded &&
        !addon.thumbnail.in_progress) {
      addon.thumbnail.in_progress = true;
      boost::asio::post(*thd_pool_, [&]() {
        const auto response = ClientFactory::GetInstance().NewSyncClient()->Get(
            addon.screenshot_url,
            {{"Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9"},
             {"Accept-Encoding", "gzip, deflate, br"}});

        if (response.data.empty()) return;

        const auto buffer_size = response.data.size();
        auto buffer = std::make_unique<uint8_t[]>(buffer_size);
        std::memcpy(buffer.get(), response.data.data(), buffer_size);

        int width, height, channels;
        auto* texture =
            LoadTexture(buffer.get(), buffer_size, &width, &height, &channels);

        if (texture != nullptr) {
          auto* resized_texture =
              ResizeTexture(texture, width, height, kThumbnailWidth,
                            kThumbnailHeight, channels);
          if (resized_texture != nullptr) {
            addon.thumbnail.pixels = resized_texture;
            addon.thumbnail.width = kThumbnailWidth;
            addon.thumbnail.height = kThumbnailHeight;
            addon.thumbnail.channels = channels;
          }
        }

        addon.thumbnail.is_loaded = true;
        addon.thumbnail.in_progress = false;
      });
    }
  }
}
void Gui::RenderInstalledTab(std::vector<InstalledAddon>& addons) {
  for (auto& addon : addons) {
    if (addon.thumbnail.is_uploaded) {
      ImGui::Image(addon.thumbnail.pixels,
                   ImVec2(addon.thumbnail.width, addon.thumbnail.height));
      ImGui::SameLine();
    }

    ImGui::Text(addon.name);
    ImGui::Text("Local: " + addon.local_version.readable_version);
    ImGui::Text("Remote: " + addon.remote_version.readable_version);

    ImGui::NewLine();

    if (addon.download_status.state != RequestState::kStatePending &&
        addon.download_status.state != RequestState::kStateFinish &&
        !addon.up_to_date) {
      ImGui::SameLine();
      if (ImGui::Button((std::string(ICON_FA_DOWNLOAD "##") +
                         addon.remote_version.readable_version)
                            .c_str())) {
        addon.Update();
      }
    }

    if (addon.download_status.state == RequestState::kStatePending) {
      ImGui::SameLine();
      ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8.0F);
      ImGui::BufferingBar("##buffer_bar",
                          addon.download_status.progress / 100.F,
                          ImVec2(200, 6));
      ImGui::SameLine();
      if (ImGui::Button(
              (std::string("Cancel ##") + addon.local_version.readable_version)
                  .c_str())) {
        addon.download_status.state = RequestState::kStateCancel;
      }
    }

    if (addon.thumbnail.is_loaded && !addon.thumbnail.is_uploaded) {
      auto* uploaded_texture =
          UploadTexture(addon.thumbnail.pixels, addon.thumbnail.width,
                        addon.thumbnail.height, addon.thumbnail.channels);

      if (uploaded_texture != nullptr) {
        addon.thumbnail.pixels = static_cast<uint8_t*>(uploaded_texture);
      }

      addon.thumbnail.is_uploaded = true;
    }

    if (ImGui::IsItemVisible() && !addon.thumbnail.is_loaded &&
        !addon.thumbnail.in_progress) {
      addon.thumbnail.in_progress = true;
      boost::asio::post(*thd_pool_, [&]() {
        const auto response = ClientFactory::GetInstance().NewSyncClient()->Get(
            addon.screenshot_url,
            {{"Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9"},
             {"Accept-Encoding", "gzip, deflate, br"}});

        if (response.ec) {
          const auto buffer_size = response.data.size();
          auto buffer = std::make_unique<uint8_t[]>(buffer_size);
          std::memcpy(buffer.get(), response.data.data(), buffer_size);

          int width, height, channels;
          auto* texture = LoadTexture(buffer.get(), buffer_size, &width,
                                      &height, &channels);

          if (texture != nullptr) {
            auto* resized_texture =
                ResizeTexture(texture, width, height, kThumbnailWidth,
                              kThumbnailHeight, channels);
            if (resized_texture != nullptr) {
              addon.thumbnail.pixels = resized_texture;
              addon.thumbnail.width = kThumbnailWidth;
              addon.thumbnail.height = kThumbnailHeight;
              addon.thumbnail.channels = channels;
            }
          }
        }
        addon.thumbnail.is_loaded = true;
        addon.thumbnail.in_progress = false;
      });
    }
  }
}
}  // namespace addon_updater