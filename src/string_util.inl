#ifndef ADDON_UPDATER_STRING_UTIL_INL
#define ADDON_UPDATER_STRING_UTIL_INL
#pragma once

namespace string_util {

inline bool Replace(std::string* base_string, std::string_view old_string,
                    std::string_view new_string) {
  const auto start = base_string->find(old_string);
  if (start == std::string::npos) {
    return false;
  }
  base_string->replace(start, old_string.length(), new_string);
  return true;
}

inline void ReplaceAll(std::string* base_string, std::string_view old_string,
                       std::string_view new_string) {
  auto position = base_string->find(old_string);
  while (position != std::string::npos) {
    base_string->replace(position, old_string.length(), new_string);
    position = base_string->find(old_string, position + new_string.length());
  }
}

inline bool Search(std::string_view target, std::string_view base_string) {
  if (target.empty() || base_string.empty()) {
    return false;
  }
  return std::search(target.begin(), target.end(), base_string.begin(),
                     base_string.end(), [](const char a, const char b) {
                       return std::tolower(a) == std::tolower(b);
                     }) != base_string.end();
}

}  // namespace string_util

#endif  // !ADDON_UPDATER_STRING_UTIL_INL
