// clang-format off
#include "pch.h"
#include "products.h"
#include "http_client.h"
#include "config.h"
#include "window.h"
#include "addon.h"
#include "texture.h"
#include "thumbnail_cache.h"
#include "gui.h"
#include "base64.h"
#include "file.h"
#include "toc_parser.h"
#include "resource_loader.h"
#include "windows_error_message.h"

#include "../data/resource/resource.h"
// clang-format on

namespace {
constexpr auto kWindowWidth = 640;
constexpr auto kWindowHeight = 480;

constexpr auto kTukuiApiUrl = "https://www.tukui.org/api.php?addons=all";

constexpr auto kCurseApiUrl =
    "https://addons-ecs.forgesvc.net/api/v2/addon/"
    "search?gameId=1&searchFilter=&pageSize=1000";

constexpr auto kTukuiClassicApiUrl =
    "https://www.tukui.org/api.php?classic-addons=all";

}  // namespace

int WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
  //#ifdef _DEBUG
  if (AllocConsole()) {
    FILE* output = nullptr;
    freopen_s(&output, "CONOUT$", "w", stdout);
    freopen_s(&output, "CONOUT$", "w", stderr);
  }
  //#endif  // !DEBUG

  const auto drive_prefix = addon_updater::GetWindowsDriveLetterPrefix();
  if (!drive_prefix.has_value()) {
    addon_updater::WindowsErrorMessageBox(
        "Error: failed to get windows drive prefix: " +
        addon_updater::WindowsErrorMessage(GetLastError()));
  }

  const auto products = addon_updater::GetProductDb(
      *drive_prefix + R"(ProgramData\Battle.net\Agent\product.db)");

  addon_updater::WowInstallations installations{};
  if (products.has_value()) {
    GetWowInstallations(*products, &installations);
  }

  addon_updater::Slugs addon_slugs{};
  if (auto resource = addon_updater::GetResource(
          ADDON_SLUGS_RESOURCE, addon_updater::ResourceType::kText);
      resource.has_value()) {
    if (!addon_updater::DeserializeAddonSlugs(
            reinterpret_cast<const char*>(resource->data), &addon_slugs)) {
      addon_updater::WindowsErrorMessageBox(
          "Error: failed to deserialize addon slugs.");
      return 1;
    }
  } else {
    addon_updater::WindowsErrorMessageBox(
        "Error: failed to get addon slugs resource.");
    return 1;
  }

  boost::asio::thread_pool thd_pool(std::thread::hardware_concurrency());

  addon_updater::Window window("", {kWindowWidth, kWindowHeight});
  addon_updater::Gui gui(thd_pool);

  auto curse_client =
      addon_updater::ClientFactory::GetInstance().NewAsyncClient();
  addon_updater::Addons addons{};
  curse_client->Get(kCurseApiUrl,
                    [&](const beast::error_code& ec, std::string_view result) {
                      if (!addon_updater::DeserializeAddons(
                              result, addon_updater::AddonType::kCurse,
                              addon_updater::AddonFlavor::kRetail, &addons)) {
                        addon_updater::WindowsErrorMessageBox(
                            "Error: failed to deserialize curse addons.");
                      }
                    });

  auto tukui_client =
      addon_updater::ClientFactory::GetInstance().NewAsyncClient();
  tukui_client->Get(kTukuiApiUrl,
                    [&](const beast::error_code& ec, std::string_view result) {
                      if (!addon_updater::DeserializeAddons(
                              result, addon_updater::AddonType::kTukui,
                              addon_updater::AddonFlavor::kRetail, &addons)) {
                        addon_updater::WindowsErrorMessageBox(
                            "Error: failed to deserialize tukui addons.");
                      }
                    });

  /*const auto tukui_classic_client =
      addon_updater::ClientFactory::GetInstance().NewAsyncClient();
  tukui_classic_client->Get(
      kTukuiClassicApiUrl,
      [&](const beast::error_code& ec, std::string_view result) {
        if (!addon_updater::DeserializeAddons(
                result, addon_updater::AddonType::kTukui,
                addon_updater::AddonFlavor::kClassic, &addons)) {
          addon_updater::WindowsErrorMessageBox(
              "Error: failed to deserialize classic tukui addons.");
        }
      });*/

  bool is_loading = true;

  addon_updater::InstalledAddons installed_addons{};
  window.Render([&](const addon_updater::WindowSize& window_size) {
    {
      const auto requests_done = curse_client->Finished() &&
                                 tukui_client->Finished();

      if (is_loading && requests_done) {
        for (auto& install : installations) {
          if (install.client_type == addon_updater::ClientType::kRetail) {
            auto result = addon_updater::DetectInstalledAddons(
                install.addons_path, addon_updater::AddonFlavor::kRetail,
                addon_slugs, addons, installed_addons);
          }
        }
        is_loading = false;
      }

      gui.DrawGui(addons, installed_addons, window_size, is_loading);
    }
  });

  thd_pool.join();
  return 0;
}
