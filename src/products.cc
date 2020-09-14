// clang-format off
#include "pch.h"
#include "products.h"
#include "string_util.h"
#include "file.h"
// clang-format on
namespace addon_updater {
namespace {
inline std::string_view ClientTypeToString(ClientType client_type) {
  switch (client_type) {
    case ClientType::kBeta:
      return "_beta_";
      break;
    case ClientType::kRetail:
      return "_retail_";
      break;
    case ClientType::kClassic:
      return "_classic_";
      break;
    case ClientType::kClassicPtr:
      return "_classic_ptr_";
      break;
    case ClientType::kRetailPtr:
      return "_ptr_";
      break;
    default:
      break;
  }
  return std::string_view();
}

inline ClientType StringToClientType(std::string_view client_string) {
  if (client_string == "_retail_") return ClientType::kRetail;
  if (client_string == "_beta_") return ClientType::kBeta;
  if (client_string == "_classic_") return ClientType::kClassic;
  if (client_string == "_classic_ptr_") return ClientType::kClassicPtr;
  if (client_string == "_ptr_") return ClientType::kRetailPtr;
  return ClientType::kRetail;
}
}  // namespace

std::optional<ProductDb> GetProductDb(std::string_view product_db_path) {
  ProductDb product_db{};

  auto result = addon_updater::ReadFile(product_db_path.data());

  if (!result.Ok()) return std::nullopt;

  if (!product_db.ParseFromString(result.content)) {
    return std::nullopt;
  }

  return product_db;
}

void GetWowInstallations(const ProductDb& product_db,
                         std::vector<WowInstallation>* detected_installations) {
  for (auto& installs : product_db.product_installs()) {
    const auto& settings = installs.settings();
    if (installs.product_code().find("wow") != std::string::npos) {
      const auto* reflect = settings.GetReflection();
      if (!reflect->GetUnknownFields(settings).empty()) {
        if (reflect->GetUnknownFields(settings).field_count() == 3) {
          const auto product_flavor =
              reflect->GetUnknownFields(settings).field(2);
          if (product_flavor.type() ==
                  google::protobuf::UnknownField::Type::TYPE_LENGTH_DELIMITED &&
              product_flavor.number() == 13) {
            const auto& flavor_string = product_flavor.length_delimited();

            auto base_path = settings.install_path() + '/' + flavor_string;
            string_util::ReplaceAll(&base_path, "/", R"(\)");
            detected_installations->push_back(
                {StringToClientType(flavor_string), base_path,
                 base_path + R"(\Interface\Addons\)", base_path + R"(\WTF\)"});
          }
        }
      }
    }
  }
}
}