// clang-format off
#include "pch.h"
#include "resource_loader.h"
// clang-format on

namespace addon_updater {
namespace {
constexpr auto kResourceType = "BINARY";

inline const char* ResourceTypeToString(ResourceType resource_type) {
  switch (resource_type) {
    case ResourceType::kBinary:
      return "BINARY";
      break;
    case ResourceType::kText:
      return "TEXT";
      break;
    default:
      break;
  }
  return "";
}
}

std::optional<Resource> GetResource(int resource_id, ResourceType type) {
  auto resource =
      FindResourceA(GetModuleHandle(nullptr), MAKEINTRESOURCEA(resource_id),
                    ResourceTypeToString(type));

  if (resource == nullptr) return {};

  Resource result{};
  result.data = reinterpret_cast<uint8_t*>(
      LockResource(LoadResource(GetModuleHandle(nullptr), resource)));
  result.size = SizeofResource(GetModuleHandle(nullptr), resource);

  return result;
}
}
