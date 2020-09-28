#ifndef ADDON_UPDATER_STRING_UTIL_H
#define ADDON_UPDATER_STRING_UTIL_H
#pragma once

#include "string_util.inl"

namespace string_util {

inline bool Replace(std::string* base_string, std::string_view old_string,
                    std::string_view new_string);

inline void ReplaceAll(std::string* base_string, std::string_view old_string,
                       std::string_view new_string);

inline bool Search(const std::string& target, const std::string& base_string);

inline std::string_view RemoveSuffixIfPresent(std::string_view s,
                                              std::string_view suffix);

inline std::string StripNonDigits(std::string_view str);
inline size_t StringToNumber(std::string_view str);

}  // namespace string_util

#endif  // !ADDON_UPDATER_STRING_UTIL_H
