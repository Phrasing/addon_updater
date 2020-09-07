// clang-format off
#include "pch.h"
#include "products.h"
#include "http_client.h"
#include "config.h"
#include "addon.h"
// clang-format on

using namespace addon_updater;

int WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
  if (AllocConsole()) {
    FILE* dummy{};
    freopen_s(&dummy, "CONOUT$", "w", stdout);
  }

  product_wrapper::ProductDbWrapper wrapper{
      R"(C:\ProgramData\Battle.net\Agent\product.db)"};

  auto installs = wrapper.GetWowInstallations();
  for (auto& install : installs) {
    std::cout << install.GetRetailAddonsPath() << std::endl;
  }

  auto config = UpdaterConfig{installs.front().GetRetailAddonsPath() +
                              R"(\WTF\addon_updater.json)"};

  config.DeserializeFromFile();

  system("pause");
  return 0;
}
