// clang-format off
#include "pch.h"
#include "http_client.h"
// clang-format on

int WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
  if (AllocConsole()) {
    FILE* dummy{};
    freopen_s(&dummy, "CONOUT$", "w", stdout);
  }



  std::getchar();
  return 0;
}

/*int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {


  system("pause");
  return 0;
}*/
