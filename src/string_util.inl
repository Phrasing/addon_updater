#ifndef ADDON_UPDATER_STRING_UTIL_INL
#define ADDON_UPDATER_STRING_UTIL_INL

namespace string_util {

inline bool Replace(std::string* base_string, std::string_view old_string,
                    std::string_view new_string) {
  if (base_string->empty() || old_string.empty()) {
    return false;
  }

  const auto start = base_string->find(old_string);
  if (start == std::string::npos) {
    return false;
  }
  base_string->replace(start, old_string.length(), new_string);
  return true;
}

inline void ReplaceAll(std::string* base_string, std::string_view old_string,
                       std::string_view new_string) {
  if (base_string->empty() || old_string.empty()) {
    return;
  }

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

inline size_t StringToNumber(std::string_view str) {
  if (str.empty()) return 0;
  return std::stoull(str.data());
}

inline std::string UrlEncodeWhitespace(std::string_view str) {
  std::string copy;
  for (size_t i = 0; i < str.length(); i++) {
    if (str[i] != ' ')
      copy += str[i];
    else
      copy += "%20";
  }
  return copy;
}

inline std::string StripNonDigits(std::string_view str) {
  auto string_copy = std::string(str.begin(), str.end());

  string_copy.erase(std::remove_if(string_copy.begin(), string_copy.end(),
                                   [](char c) {
                                     if (static_cast<unsigned char>(c) > 127)
                                       return true;

                                     return !std::isdigit(c);
                                   }),
                    string_copy.end());

  return string_copy;
}

inline std::string_view RemoveSuffixIfPresent(std::string_view s,
                                              std::string_view suffix) {
  if (s.ends_with(suffix)) {
    s.remove_suffix(suffix.size());
  }
  return s;
}

}  // namespace string_util

#endif  // !ADDON_UPDATER_STRING_UTIL_INL
