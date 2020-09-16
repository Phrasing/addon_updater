// clang-format off
#include "pch.h"
#include "gui.h"
#include "texture.h"
#include "http_client.h"
// clang-format on
namespace addon_updater {

namespace {
constexpr auto kThumbnailWidth = 32;
constexpr auto kThumbnailHeight = 32;
}  // namespace

Gui::Gui(boost::asio::thread_pool* thd_pool) : thd_pool_(thd_pool) {}

void Gui::DrawGui(std::vector<addon_updater::Addon>& addons,
                  const std::pair<int32_t, int32_t>& window_size) {
  ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
  ImGui::SetNextWindowSize(ImVec2(window_size.first, window_size.second));
  ImGui::Begin("##MAIN", nullptr,
               ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
  {
    if (ImGui::BeginTabBar("##MAIN_TAB_BAR")) {
      if (ImGui::BeginTabItem("Browse")) {
        this->RenderBrowseTab(addons);
        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Installed")) {
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
    ImGui::Text(addon.stripped_version);

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
            addon.screenshot_url);

        if (response.ec) {
          const auto buffer_size = response.data.size();
          auto buffer = std::make_unique<uint8_t[]>(buffer_size);
          std::memcpy(buffer.get(), response.data.data(), buffer_size);

          int32_t width = 0;
          int32_t height = 0;
          int32_t channels = 0;
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