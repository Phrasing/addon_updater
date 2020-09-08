// clang-format off
#include "pch.h"
#include "products.h"
#include "http_client.h"
#include "config.h"
#include "window.h"
#include "addon.h"
// clang-format on

using namespace addon_updater;

int WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
#ifdef _DEBUG
  if (AllocConsole()) {
    FILE* dummy{};
    freopen_s(&dummy, "CONOUT$", "w", stdout);
    freopen_s(&dummy, "CONOUT$", "w", stderr);
  }
#endif  // !DEBUG

  auto& products = product_wrapper::ProductDbWrapper::GetInstance();
  products.LoadProtoDbFile(R"(C:\ProgramData\Battle.net\Agent\product.db)");

  auto installs = products.GetWowInstallations();
  for (auto& install : installs) {
    std::cout << install.GetRetailAddonsPath() << std::endl;
  }

  auto window = addon_updater_window::Window("Test", {1280, 720});

  bool show_demo_window = true;
  bool show_another_window = false;

  window.Render([&](const std::pair<int32_t, int32_t>& window_size) {
    {
      ImGui::SetNextWindowPos(ImVec2(0, 0));
      ImGui::SetNextWindowSize(ImVec2(window_size.first, window_size.second));
      ImGui::Begin("Test");
      ImGui::Text("test");
      ImGui::End();
    }
  });

  /*auto config = UpdaterConfig{installs.front().GetRetailAddonsPath() +
                              R"(\WTF\addon_updater.json)"};

  if (!config.DeserializeFromFile()) {
    static_cast<void>(::MessageBoxA(
        nullptr, "Failed to deserialize config file.\nGenerate a new one?",
        nullptr, MB_ICONQUESTION));
  }*/

  /*  auto client = http_client::ClientFactory::GetInstance().NewSyncClient();
    const auto result = client->Get(
        "https://addons-ecs.forgesvc.net/api/v2/addon/"
        "search?gameId=1&searchFilter=&pageSize=1000");

    const auto addons = DeserializeAddons(result.data, AddonType::kCurse);
    for (auto& addon : addons) {
      std::cout << addon.name << std::endl;
    }*/

  // system("pause");
  return 0;
}
