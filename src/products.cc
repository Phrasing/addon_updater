// clang-format off
#include "pch.h"
#include "products.h"
// clang-format on

product_wrapper::ProductDbWrapper::ProductDbWrapper(
    const std::string_view file_path) {
  auto istream = std::make_unique<std::fstream>(
      file_path.data(), std::ios::binary | std::ios::in);
  if (!product_db_.ParseFromIstream(istream.get())) {
    //@TODO: Error Handling
  }
}

std::vector<product_wrapper::WowInstallation>
product_wrapper::ProductDbWrapper::GetWowInstallations() {
  std::vector<WowInstallation> wow_installations{};

  if (!product_db_.IsInitialized()) return wow_installations;

  for (auto& installs : product_db_.product_installs()) {
    if (installs.product_code() == "wow") {
      wow_installations.push_back({installs.settings().install_path()});
    }
  }

  return wow_installations;
}

std::optional<std::string>
product_wrapper::WowInstallation::GetClassicAddonsPath() {
  const auto path = base_path + R"(\_classic_)";
  return std::filesystem::exists(path) ? std::optional<std::string>(path)
                                       : std::nullopt;
}

std::optional<std::string>
product_wrapper::WowInstallation::GetRetailAddonsPath() {
  const auto path = base_path + R"(\_retail_)";
  return std::filesystem::exists(path) ? std::optional<std::string>(path)
                                       : std::nullopt;
}

std::optional<std::string>
product_wrapper::WowInstallation::GetRetailPtrAddonsPath() {
  const auto path = base_path + R"(\_ptr_)";
  return std::filesystem::exists(path) ? std::optional<std::string>(path)
                                       : std::nullopt;
}

std::optional<std::string>
product_wrapper::WowInstallation::GetClassicPtrAddonsPath() {
  const auto path = base_path + R"(\_classic_ptr_)";
  return std::filesystem::exists(path) ? std::optional<std::string>(path)
                                       : std::nullopt;
}

std::optional<std::string>
product_wrapper::WowInstallation::GetBetaAddonsPath() {
  const auto path = base_path + R"(\_beta_)";
  return std::filesystem::exists(path) ? std::optional<std::string>(path)
                                       : std::nullopt;
}
