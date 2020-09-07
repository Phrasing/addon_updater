// clang-format off
#include "pch.h"
#include "products.h"
#include "string_util.h"
// clang-format on

bool product_wrapper::ProductDbWrapper::LoadProtoDbFile(
    std::string_view file_path) {
  auto istream = std::make_unique<std::fstream>(
      file_path.data(), std::ios::binary | std::ios::in);
  if (!product_db_.ParseFromIstream(istream.get())) {
    //@TODO: Error Handling
    return false;
  }
  return true;
}

std::vector<product_wrapper::WowInstallation>
product_wrapper::ProductDbWrapper::GetWowInstallations() {
  std::vector<WowInstallation> wow_installations{};

  if (!product_db_.IsInitialized()) return wow_installations;

  for (auto& installs : product_db_.product_installs()) {
    if (installs.product_code() == "wow") {
      auto install_path = installs.settings().install_path();
      string_util::ReplaceAll(&install_path, "/", R"(\)");
      wow_installations.push_back({install_path});
    }
  }

  return wow_installations;
}

std::string product_wrapper::WowInstallation::GetClassicAddonsPath() {
  const auto path = base_path + R"(\_classic_)";
  return std::filesystem::exists(path) ? path : std::string();
}

std::string product_wrapper::WowInstallation::GetRetailAddonsPath() {
  const auto path = base_path + R"(\_retail_)";
  return std::filesystem::exists(path) ? path : std::string();
}

std::string product_wrapper::WowInstallation::GetRetailPtrAddonsPath() {
  const auto path = base_path + R"(\_ptr_)";
  return std::filesystem::exists(path) ? path : std::string();
}

std::string product_wrapper::WowInstallation::GetClassicPtrAddonsPath() {
  const auto path = base_path + R"(\_classic_ptr_)";
  return std::filesystem::exists(path) ? path : std::string();
}

std::string product_wrapper::WowInstallation::GetBetaAddonsPath() {
  const auto path = base_path + R"(\_beta_)";
  return std::filesystem::exists(path) ? path : std::string();
}
