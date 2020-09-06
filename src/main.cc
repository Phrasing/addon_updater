// clang-format off
#include "pch.h"
#include "products.h"
#include "http_client.h"
// clang-format on

int WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
  if (AllocConsole()) {
    FILE* dummy{};
    freopen_s(&dummy, "CONOUT$", "w", stdout);
  }

  product_wrapper::ProductDbWrapper wrapper{
      R"(C:\ProgramData\Battle.net\Agent\product.db)"};

  auto installs = wrapper.GetWowInstallations();
  for (auto& install : installs) {
    std::cout << install.GetRetailAddonsPath().value() << std::endl;
  }

  auto client = http_client::ClientFactory::GetInstance().NewAsyncClient();
  client->Verbose(true);
  client->Get(
      "https://addons-ecs.forgesvc.net/api/v2/addon/"
      "search?gameId=1&searchFilter=",
      [&](const beast::error_code& ec, std::string_view data) {
        //std::cout << data << std::endl;
      });

  system("pause");
  return 0;
}
