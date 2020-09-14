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

struct WowInstallation {
  ClientType client_type;
  std::string base_path;
  std::string addons_path;
  std::string wtf_path;
};

std::optional<ProductDb> GetProductDb(std::string_view product_db_path);
void GetWowInstallations(const ProductDb& product_db,
                         std::vector<WowInstallation>* installs);


}  // namespace addon_updater_products

#endif  // !PRODUCTS_H
