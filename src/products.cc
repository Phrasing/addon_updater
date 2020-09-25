// clang-format off
#include "pch.h"
#include "products.h"
#include "string_util.h"
#include "file.h"
// clang-format on

namespace addon_updater {
namespace {
constexpr auto kFieldCount = 3;
constexpr auto kProductFlavor = 2;
constexpr auto kFieldFlavorId = 13;

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

std::optional<WowInstallations> GetWowInstallations(
    const ProductDb& product_db) {
  WowInstallations installs{};

  if (product_db.product_installs().empty()) return std::nullopt;

  for (const auto& product_install : product_db.product_installs()) {
    const auto& settings = product_install.settings();

    if (product_install.product_code().find("wow") == std::string::npos)
      continue;

    const auto* reflection = UserSettings::GetReflection();
    if (reflection->GetUnknownFields(settings).empty() ||
        reflection->GetUnknownFields(settings).field_count() != kFieldCount)
      continue;

    const auto product_flavor =
        reflection->GetUnknownFields(settings).field(kProductFlavor);

    if (product_flavor.type() !=
            google::protobuf::UnknownField::Type::TYPE_LENGTH_DELIMITED ||
        product_flavor.number() != kFieldFlavorId)
      continue;

    const auto& flavor_string = product_flavor.length_delimited();

    auto base_path = settings.install_path() + '/' + flavor_string;
    string_util::ReplaceAll(&base_path, "/", R"(\)");

    const auto addons_path = base_path + R"(\Interface\Addons\)";
    const auto wtf_path = base_path + R"(\WTF\)";

    const auto client_type = StringToClientType(flavor_string);
    switch (client_type) {
      case ClientType::kRetail: {
        installs.retail = {client_type, base_path, addons_path, wtf_path};
      } break;
      case ClientType::kClassic: {
        installs.classic = {client_type, base_path, addons_path, wtf_path};
      } break;
      case ClientType::kRetailPtr: {
        installs.retail_ptr = {client_type, base_path, addons_path, wtf_path};
      } break;
      case ClientType::kBeta: {
        installs.beta = {client_type, base_path, addons_path, wtf_path};
      } break;
      case ClientType::kClassicPtr: {
        installs.classic_ptr = {client_type, base_path, addons_path, wtf_path};
      } break;
      default:
        break;
    }
  }
  return installs;
}

}