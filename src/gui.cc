// clang-format off
#include "pch.h"
#include "gui.h"
#include "texture.h"
#include "file.h"
#include "http_client.h"
#include "resource_loader.h"
#include "../data/resource/resource.h"
// clang-format on

namespace ImGui {

bool BufferingBar(const char* label, float value, const ImVec2& size_arg) {
  auto* window = GetCurrentWindow();
  if (window->SkipItems) return false;

  const ImU32 fg_col = ImGui::GetColorU32(ImGuiCol_ButtonActive);
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

  window->DrawList->PathStroke(ImGui::GetColorU32(ImGuiCol_ButtonActive), false,
                               thickness);
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
    ImGui::Spinner("##loading_spinner", spinner_size, 20);
  }
  ImGui::End();
}

}  // namespace

Gui::Gui(boost::asio::thread_pool& thd_pool,
         const WowInstallations& installations)
    : thd_pool_(&thd_pool), installations_(installations) {
  if (const auto curse_resource = addon_updater::GetResource(
          DEFAULT_ADDON_ICON, addon_updater::ResourceType::kBinary);
      curse_resource.has_value()) {
    this->curse_icon_ = curse_resource->data;
    this->curse_icon_size_ = curse_resource->size;
  }

  for (const auto& install : this->installations_.GetContainer()) {
    if (!install.addons_path.empty()) {
      this->selected_installation_ = install;
      break;
    }
  }
}

void Gui::DrawGui(Addons& addons, std::vector<InstalledAddon>& installed_addons,
                  const WindowSize& window_size, bool is_loading) {
  ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
  ImGui::SetNextWindowSize(ImVec2(window_size.width, window_size.height));

  if (is_loading) {
    RenderLoadingScreen(window_size);
    return;
  }

  static bool is_open = true;
  ImGui::Begin("Addon Updater v1.0.1", &is_open,
               ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_Minimize |
                   ImGuiWindowFlags_NoScrollbar);
  {
    auto browse = false;
    auto installed = false;

    auto& style = ImGui::GetStyle();
    ImGuiComboFlags flags = ImGuiComboFlags_NoArrowButton;

    float w = ImGui::CalcItemWidth();
    float spacing = style.ItemInnerSpacing.x;
    float button_sz = ImGui::GetFrameHeight();
    ImGui::PushItemWidth(w - spacing * 2.0f - button_sz * 2.0f);
    if (ImGui::BeginCombo("##custom combo",
                          selected_installation_.addons_path.c_str(),
                          ImGuiComboFlags_NoArrowButton)) {
      for (auto& install : this->installations_.GetContainer()) {
        bool is_selected =
            (selected_installation_.addons_path == install.addons_path);
        if (ImGui::Selectable(install.addons_path.c_str(), is_selected))
          selected_installation_ = install;
        if (is_selected) ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }

    ImGui::PopItemWidth();
    ImGui::SameLine(0, spacing);
    if (ImGui::ArrowButton("##r", ImGuiDir_Left)) {
    }
    ImGui::SameLine(0, spacing);
    if (ImGui::ArrowButton("##r", ImGuiDir_Right)) {
    }

    float tab_sz = ImGui::GetFrameHeight();
    if (ImGui::BeginTabBar("##MAIN_TAB_BAR")) {
      if (browse = ImGui::BeginTabItem("Browse  " ICON_FA_SEARCH)) {
        ImGui::EndTabItem();
      }

      if (installed = ImGui::BeginTabItem("Installed  " ICON_FA_FOLDER)) {
        ImGui::EndTabItem();
      }
    }
    ImGui::SameLine();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 2.5f);

    float w_ = ImGui::CalcItemWidth();
    float spacing_ = style.ItemInnerSpacing.x;
    float button_sz_ = ImGui::GetFrameHeight();

    ImGui::PushItemWidth(w_ - spacing_ * 1.5f - button_sz_ * 1.5f);

    ImGui::InputText("##Search", &search_text_);
    ImGui::PopItemWidth();
    ImGui::EndTabBar();

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
    ImGui::BeginChild(
        "##MAIN", ImVec2(window_size.width - 8, window_size.height - 90.F),
        false,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_Minimize);
    {
      if (installed) {
        this->RenderInstalledTab(installed_addons);
      } else if (browse) {
        this->RenderBrowseTab(addons, installed_addons);
      }
    }
    ImGui::EndChild();
  }
  ImGui::End();
}

void Gui::RenderBrowseTab(
    std::vector<addon_updater::Addon>& addons,
    std::vector<addon_updater::InstalledAddon>& installed_addons) {
  for (auto& addon : addons) {
    const auto found =
        std::find_if(installed_addons.begin(), installed_addons.end(),
                     [&addon](const InstalledAddon& installed_addon) -> bool {
                       return installed_addon.id == addon.id;
                     });
    if (found != installed_addons.end()) {
      // If the addon is currently being downloaded we don't want to remove it
      // just yet.
      addon.is_ignored =
          !(addon.download_status.state == RequestState::kStatePending);
    } else {
      // Assume that the addon has been uninstalled and check to see if we need
      // to change the request state to allow downloading the addon again.
      addon.is_ignored = false;
      if (addon.download_status.state == RequestState::kStateCancel ||
          addon.download_status.state == RequestState::kStateFinish) {
        addon.download_status.state = RequestState::kStateNone;
      }
    }

    auto search_result =
        std::search(addon.name.begin(), addon.name.end(), search_text_.begin(),
                    search_text_.end(), [](const char a, const char b) {
                      return std::tolower(a) == std::tolower(b);
                    });

    auto filter =
        !this->search_text_.empty() && search_result == addon.name.end();

    if (filter) {
      continue;
    }

    if (addon.is_ignored) continue;

    if (addon.thumbnail.is_uploaded) {
      ImGui::Image(addon.thumbnail.pixels,
                   ImVec2(addon.thumbnail.width, addon.thumbnail.height));
      ImGui::SameLine();
    }

    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0, 0, 0, 0));
    if (ImGui::TreeNode(
            (addon.name + "##" + std::to_string(addon.id)).c_str())) {
      ImGui::TextWrapped((std::string("Description: ") + addon.description).c_str());
      ImGui::Text("Flavor: " + FlavorToString(addon.flavor));
      ImGui::TreePop();
    }
    ImGui::PopStyleColor(2);

    if (addon.download_status.state != RequestState::kStatePending &&
        addon.download_status.state != RequestState::kStateFinish) {
      ImGui::SameLine();
      ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - 50);
      if (ImGui::Button((std::string(ICON_FA_DOWNLOAD "##") +
                         addon.remote_version.readable_version)
                            .c_str())) {
        const auto installed_addon =
            addon.Install(installations_.retail.addons_path);
        installed_addons.push_back(installed_addon);
      }
    }

    if (addon.download_status.state == RequestState::kStatePending) {
      ImGui::SameLine();
      ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8.0F);
      ImGui::BufferingBar("##buffer_bar",
                          addon.download_status.progress / 100.F,
                          ImVec2(200, 6));
      ImGui::SameLine();
      ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 50);
      ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 10);
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
      AsyncLoadAddonThumbnail(&addon.thumbnail, addon.screenshot_url);
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

    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0, 0, 0, 0));
    if (ImGui::TreeNode(
            (addon.name + "##" + std::to_string(addon.id)).c_str())) {
      ImGui::TextColored(
          ImVec4(1.0f, 0.0f, 1.0f, 1.0f),
          ("Local: " + addon.local_version.readable_version).c_str());
      ImGui::TextColored(
          ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
          ("Remote: " + addon.remote_version.readable_version).c_str());

      if (ImGui::Button("Uninstall")) {
        addon.Uninstall();
        if (auto position = std::find(addons.begin(), addons.end(), addon);
            position != addons.end()) {
          addons.erase(position);
        }
        ImGui::TreePop();
        ImGui::PopStyleColor(2);
        continue;
      }

      ImGui::TreePop();
    }
    ImGui::PopStyleColor(2);

    if (addon.download_status.state != RequestState::kStatePending &&
        addon.download_status.state != RequestState::kStateFinish &&
        !addon.up_to_date) {
      ImGui::SameLine();
      ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - 25);
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
      ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 50);
      ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 10);
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
      AsyncLoadAddonThumbnail(&addon.thumbnail, addon.screenshot_url);
    }
  }
}

void Gui::AsyncLoadAddonThumbnail(AddonThumbnail* thumbnail,
                                  std::string_view screenshot_url) {
  thumbnail->in_progress = true;
  boost::asio::post(*thd_pool_, [screenshot_url, thumbnail, this]() {
    if (screenshot_url.empty()) {
    load_default:
      if (this->LoadAndResizeThumbnail(curse_icon_, curse_icon_size_,
                                       thumbnail)) {
        thumbnail->is_loaded = true;
        thumbnail->in_progress = false;
      }
      return;
    }

    ClientFactory::GetInstance().NewAsyncClient()->Get(
        screenshot_url, [this, thumbnail](const beast::error_code& ec,
                                          std::string_view response) {
          if (response.empty()) {
            if (this->LoadAndResizeThumbnail(curse_icon_, curse_icon_size_,
                                             thumbnail)) {
              thumbnail->is_loaded = true;
              thumbnail->in_progress = false;
              return;
            }
          }

          const auto buffer_size = response.size();
          auto buffer = std::make_unique<uint8_t[]>(buffer_size);
          std::memcpy(buffer.get(), response.data(), buffer_size);

          if (LoadAndResizeThumbnail(buffer.get(), buffer_size, thumbnail)) {
            thumbnail->is_loaded = true;
            thumbnail->in_progress = false;
          } else {
            if (this->LoadAndResizeThumbnail(curse_icon_, curse_icon_size_,
                                             thumbnail)) {
              thumbnail->is_loaded = true;
              thumbnail->in_progress = false;
              return;
            }
          }
        });
  });
}

bool Gui::LoadAndResizeThumbnail(uint8_t* data, size_t data_size,
                                 addon_updater::AddonThumbnail* thumbnail) {
  int width, height, channels;
  auto* texture = LoadTexture(data, data_size, &width, &height, &channels);

  if (!texture) return false;

  auto* resized_texture =
      ResizeTexture(texture, std::exchange<int>(width, kThumbnailWidth),
                    std::exchange<int>(height, kThumbnailHeight),
                    kThumbnailWidth, kThumbnailHeight, channels);

  if (!resized_texture) return false;

  thumbnail->pixels = resized_texture;
  thumbnail->width = width;
  thumbnail->height = height;
  thumbnail->channels = channels;

  return true;
}
}  // namespace addon_updater