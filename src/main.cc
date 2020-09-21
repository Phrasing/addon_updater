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
  auto products = addon_updater::GetProductDb(
      R"(C:\ProgramData\Battle.net\Agent\product.db)");

  if (products != std::nullopt) {
    GetWowInstallations((products.value()), &wow_installs);
  }

  addon_updater::Slugs addon_slugs{};
  if (auto resource = addon_updater::GetResource(
          ADDON_SLUGS_RESOURCE, addon_updater::ResourceType::kText);
      resource.has_value()) {
    if (!addon_updater::DeserializeAddonSlugs(
            reinterpret_cast<const char*>(resource->data),
            &addon_slugs)) {
      std::fprintf(stderr, "Error: failed to deserialize addon slugs.\n");
      system("pause");
      return 1;
    }

  } else {
    std::cout << "Failed\n";
  }

  boost::asio::thread_pool thd_pool(std::thread::hardware_concurrency());

  auto window = addon_updater::Window("Test", {kWindowWidth, kWindowHeight});
  auto gui = addon_updater::Gui(&thd_pool);

  auto result =
      addon_updater::ClientFactory::GetInstance().NewSyncClient()->Get(
          "https://addons-ecs.forgesvc.net/api/v2/addon/"
          "search?gameId=1&searchFilter=&pageSize=500");

  auto tukui = addon_updater::ClientFactory::GetInstance().NewSyncClient()->Get(
      "https://www.tukui.org/api.php?addons=all");

  auto tukui_classic =
      addon_updater::ClientFactory::GetInstance().NewSyncClient()->Get(
          "https://www.tukui.org/api.php?classic-addons=all");

  if (!result.ec || result.data.empty()) {
    std::fprintf(stderr, "Error: failed to retrieve curse repository.\n");
    system("pause");
    return 1;
  }

  addon_updater::Addons addons{};
  if (!addon_updater::DeserializeAddons(
          result.data, addon_updater::AddonType::kCurse,
          addon_updater::AddonFlavor::kRetail, &addons)) {
    std::fprintf(stderr, "Error: failed to deserialize curse addons.\n");
    system("pause");
    return 1;
  }

  addon_updater::InstalledAddons installed_addons{};
  for (auto& install : wow_installs) {
    if (install.client_type == addon_updater::ClientType::kRetail) {
      auto result = addon_updater::DetectInstalledAddons(
          install.addons_path, addon_updater::AddonFlavor::kRetail, addon_slugs,
          addons, installed_addons);
    }
  }

  if (!addon_updater::DeserializeAddons(
          tukui.data, addon_updater::AddonType::kTukui,
          addon_updater::AddonFlavor::kRetail, &addons)) {
    std::fprintf(stderr, "Error: failed to deserialize tukui addons.\n");
    system("pause");
    return 1;
  }

  if (!addon_updater::DeserializeAddons(
          tukui_classic.data, addon_updater::AddonType::kTukui,
          addon_updater::AddonFlavor::kClassic, &addons)) {
    std::fprintf(stderr,
                 "Error: failed to deserialize classic tukui addons.\n");
    system("pause");
    return 1;
  }

  for (auto& install : installed_addons) {
    std::cout << install.name << std::endl;
    for (auto& dir : install.directories) {
      std::cout << "{" << dir << "}, \n";
    }
    std::cout << "\n";
    //  std::cout << slug.addon_name << " : " << slug.slug_name << std::endl;
  }

  window.Render([&](const addon_updater::WindowSize& window_size) {
    { gui.DrawGui(addons, installed_addons, window_size); }
  });

  thd_pool.join();

  // system("pause");
  return 0;
}
