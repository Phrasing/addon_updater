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
    "search?gameId=1&searchFilter=&pageSize=10000";

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

  if (!products.has_value()) {
    std::fprintf(stderr, "Error: failed to detect wow installations.\n");
    return 1;
  }

  auto installations = addon_updater::GetWowInstallations(*products);

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
  addon_updater::Gui gui(thd_pool, *installations);

  auto curse_client =
      addon_updater::ClientFactory::GetInstance().NewAsyncClient();

  addon_updater::Addons addons{};
  curse_client->Verbose(true);
  curse_client->Get(kCurseApiUrl,
                    [&](const beast::error_code& ec, std::string_view result) {
                      if (!addon_updater::DeserializeAddons(
                              result, addon_updater::AddonType::kCurse,
                              addon_updater::AddonFlavor::kRetail, &addons)) {
                        addon_updater::WindowsErrorMessageBox(
                            "Error: failed to deserialize curse addons.");
                      }
                      std::cout << addons.size() << std::endl;
                    },
                    {{http::field::accept, "application/json"},
                     {http::field::accept_encoding, "gzip, deflate, br"}});
  
  bool is_loading = true;

  addon_updater::InstalledAddons installed_addons{};
  window.Render([&](const addon_updater::WindowSize& window_size) {
    {
      const auto requests_done = curse_client->Finished();

      if (is_loading && requests_done) {
        auto result = addon_updater::DetectInstalledAddons(
            installations->retail.addons_path,
            addon_updater::AddonFlavor::kRetail, addon_slugs, addons,
            installed_addons);
        is_loading = false;
      }

      gui.DrawGui(addons, installed_addons, window_size, is_loading);
    }
  });

  thd_pool.join();
  return 0;
}
