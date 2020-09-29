#ifndef PRODUCTS_H
#define PRODUCTS_H
#pragma once

namespace addon_updater {

enum class ClientType {
  kClassic,
  kClassicPtr,
  kRetail,
  kRetailPtr,
  kBeta,
};

struct WowInstall {
  std::string base_path;
  std::string addons_path;
  std::string wtf_path;
};

struct WowInstallations {
  WowInstall retail;
  WowInstall classic;
  WowInstall beta;
  WowInstall retail_ptr;
  WowInstall classic_ptr;

  std::vector<WowInstall> GetContainer() {
    return std::vector<WowInstall>{retail, classic, beta, retail_ptr,
                                   classic_ptr};
  }
};

std::optional<ProductDb> GetProductDb(std::string_view product_db_path);
std::optional<WowInstallations> GetWowInstallations(
    const ProductDb& product_db);

}  // namespace addon_updater_products

#endif  // !PRODUCTS_H
