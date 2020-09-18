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

  for (auto& install : wow_installs) {
    auto toc_file = addon_updater::ParseTocFile(install.addons_path +
                                                R"(BigWigs\BigWigs.toc)");
    if (toc_file != std::nullopt) {
      std::cout << toc_file.value().numeric_version << std::endl;
    }
  }

  if (auto resource = addon_updater::GetResource(
          IDR_SLUGS, addon_updater::ResourceType::kText);
      resource.has_value()) {
    std::cout << reinterpret_cast<const char*>(resource.value().data)
              << std::endl;
  } else {
    std::cout << "Failed\n";
  }

  boost::asio::thread_pool thd_pool(std::thread::hardware_concurrency());

  auto window = addon_updater::Window("Test", {kWindowWidth, kWindowHeight});
  auto gui = addon_updater::Gui(&thd_pool);

  bool show_demo_window = true;
  bool show_another_window = false;

  const auto client =
      addon_updater::ClientFactory::GetInstance().NewSyncClient();

  auto result = client->Get(
      "https://addons-ecs.forgesvc.net/api/v2/addon/"
      "search?gameId=1&searchFilter=&pageSize=500");

  if (!result.ec || result.data.empty()) {
    std::fprintf(stderr, "Error: failed to retrieve curse repository.\n");
    system("pause");
    return 1;
  }

  addon_updater::AddonVect addons{};
  if (!addon_updater::DeserializeAddons(
          result.data, addon_updater::AddonType::kCurse, &addons)) {
    std::fprintf(stderr, "Error: failed to deserialize curse addons.\n");
    system("pause");
    return 1;
  }

  window.Render([&](const addon_updater::WindowSize& window_size) {
    { gui.DrawGui(addons, window_size); }
  });

  thd_pool.join();

  // system("pause");
  return 0;
}
