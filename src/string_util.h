#ifndef ADDON_UPDATER_STRING_UTIL_H
#define ADDON_UPDATER_STRING_UTIL_H
#pragma once

#include "string_util.inl"

namespace string_util {

inline bool Replace(std::string* base_string, std::string_view old_string,
                    std::string_view new_string);

inline void ReplaceAll(std::string* base_string, std::string_view old_string,
                       std::string_view new_string);

inline bool Search(std::string_view target, std::string_view base_string);

}  // namespace string_util

#endif  // !ADDON_UPDATER_STRING_UTIL_H
