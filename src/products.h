#ifndef PRODUCTS_H
#define PRODUCTS_H
#pragma once

namespace product_wrapper {

enum class ClientType {
  kClassic,
  kClassicPtr,
  kRetail,
  kRetailPtr,
  kBeta,
};

struct WowInstallation {
  std::string GetClassicAddonsPath();
  std::string GetRetailAddonsPath();
  std::string GetRetailPtrAddonsPath();
  std::string GetClassicPtrAddonsPath();
  std::string GetBetaAddonsPath();

  std::string base_path;
};

class ProductDbWrapper : public Singleton<ProductDbWrapper> {
 public:
  ProductDbWrapper() = default;
  ~ProductDbWrapper() = default;


  bool LoadProtoDbFile(std::string_view file_path);
  std::vector<WowInstallation> GetWowInstallations();

 private:
  ProductDb product_db_;
};

}  // namespace product_wrapper

#endif  // !PRODUCTS_H
