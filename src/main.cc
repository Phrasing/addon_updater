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
#include "../data/resource/resource.h"
// clang-format on

namespace {
constexpr auto kWindowWidth = 640;
constexpr auto kWindowHeight = 480;

constexpr auto kTukuiApiUrl = "https://www.tukui.org/api.php?addons=all";
constexpr auto kCurseApiUrl =
    "https://addons-ecs.forgesvc.net/api/v2/addon/"
    "search?gameId=1&searchFilter=&pageSize=500";
constexpr auto kTukuiClassicApiUrl =
    "https://www.tukui.org/api.php?classic-addons=all";

}  // namespace

int WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
  //#ifdef _DEBUG
  if (AllocConsole()) {
    FILE* dummy{};
    freopen_s(&dummy, "CONOUT$", "w", stdout);
    freopen_s(&dummy, "CONOUT$", "w", stderr);
  }
  //#endif  // !DEBUG

  std::vector<addon_updater::WowInstallation> wow_installs{};
  auto products =
      addon_updater::GetProductDb(addon_updater::GetWindowsDriveLetterPrefix() +
                                  R"(ProgramData\Battle.net\Agent\product.db)");

  if (products != std::nullopt) {
    GetWowInstallations((products.value()), &wow_installs);
  }

  addon_updater::Slugs addon_slugs{};
  if (auto resource = addon_updater::GetResource(
          ADDON_SLUGS_RESOURCE, addon_updater::ResourceType::kText);
      resource.has_value()) {
    if (!addon_updater::DeserializeAddonSlugs(
            reinterpret_cast<const char*>(resource->data), &addon_slugs)) {
      std::fprintf(stderr, "Error: failed to deserialize addon slugs.\n");
      system("pause");
      return 1;
    }
  } else {
    std::fprintf(stderr, "Error: failed to get addon slugs resource.\n");
  }

  boost::asio::thread_pool thd_pool(std::thread::hardware_concurrency());

  auto window = addon_updater::Window("Test", {kWindowWidth, kWindowHeight});
  auto gui = addon_updater::Gui(&thd_pool);

  addon_updater::Addons addons{};

  auto curse_client =
      addon_updater::ClientFactory::GetInstance().NewAsyncClient();

  curse_client->Get(
      kCurseApiUrl, [&](const beast::error_code& ec, std::string_view result) {
        if (!addon_updater::DeserializeAddons(
                result, addon_updater::AddonType::kCurse,
                addon_updater::AddonFlavor::kRetail, &addons)) {
          std::fprintf(stderr, "Error: failed to deserialize curse addons.\n");
          system("pause");
        }
      });

  auto tukui_client =
      addon_updater::ClientFactory::GetInstance().NewAsyncClient();
  tukui_client->Get(kTukuiApiUrl, [&](const beast::error_code& ec,
                                      std::string_view result) {
    if (!addon_updater::DeserializeAddons(
            result, addon_updater::AddonType::kTukui,
            addon_updater::AddonFlavor::kRetail, &addons)) {
      std::fprintf(stderr, "Error: failed to deserialize tukui   addons.\n");
      system("pause");
    }
  });

  auto tukui_classic_client =
      addon_updater::ClientFactory::GetInstance().NewAsyncClient();
  tukui_classic_client->Get(
      kTukuiClassicApiUrl,
      [&](const beast::error_code& ec, std::string_view result) {
        if (!addon_updater::DeserializeAddons(
                result, addon_updater::AddonType::kTukui,
                addon_updater::AddonFlavor::kClassic, &addons)) {
          std::fprintf(
              stderr, "Error: failed to deserialize classic tukui   addons.\n");
          system("pause");
        }
      });

  bool is_loading = true;

  addon_updater::InstalledAddons installed_addons{};
  window.Render([&](const addon_updater::WindowSize& window_size) {
    {
      const auto requests_done = curse_client->Finished() &&
                                 tukui_client->Finished() &&
                                 tukui_classic_client->Finished();

      if (is_loading && requests_done) {
        for (auto& install : wow_installs) {
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

  // system("pause");
  return 0;
}
