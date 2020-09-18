#ifndef ADDON_UPDATER_RESOURCE_LOADER_H
#define ADDON_UPDATER_RESOURCE_LOADER_H

namespace addon_updater {

enum class ResourceType {
    kBinary,
    kText
};


struct Resource {
  uint8_t* data;
  int size;
};

std::optional<Resource> GetResource(int resource_id, ResourceType type);
}

#endif  // !ADDON_UPDATER_RESOURCE_LOADER_H
