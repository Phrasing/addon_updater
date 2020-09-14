// clang-format off
#include "pch.h"
#include "file.h"
#include "toc_parser.h"
// clang-format on

namespace addon_updater {
namespace {
inline void StripColorCharacters(std::string& str) {
  str = boost::regex_replace(
      str, std::move(boost::regex(R"(\|[a-zA-Z0-9]{9})")), "");
}

inline void StripNewLineCharacters(std::string& str) {
  str = boost::regex_replace(str, std::move(boost::regex(R"(\|r)")), "");
}

inline std::string GetTocValue(std::string_view key,
                               const std::string& toc_data) {
  boost::match_results<std::string::const_iterator> results;
  if (boost::regex_search(
          static_cast<std::string::const_iterator>(toc_data.begin()),
          static_cast<std::string::const_iterator>(toc_data.end()), results,
          boost::regex("^## " + std::string(key) + ":(.*?)$"),
          boost::match_default)) {
    auto result = std::string(results[1].first, results[1].second);
    StripColorCharacters(result);
    StripNewLineCharacters(result);
    if (result.at(0) == ' ') {
      result.erase(0, 1);
    }
    return result;
  }
  return std::string();
}
}

std::optional<TocFile> addon_updater::ParseTocFile(std::string_view file_path) {
  auto result = addon_updater::ReadFile(file_path.data());

  if (!result.Ok()) return {};

  TocFile toc_file{};
  toc_file.author = std::move(GetTocValue("Author", result.content));
  toc_file.title = std::move(GetTocValue("Title", result.content));
  toc_file.notes = std::move(GetTocValue("Notes", result.content));

  auto readable_version = std::move(GetTocValue("Version", result.content));

  toc_file.readable_version = readable_version;

  readable_version.erase(
      std::remove_if(readable_version.begin(), readable_version.end(),
                     [](uint8_t c) { return !std::isdigit(c); }),
      readable_version.end());
  
  toc_file.numeric_version = std::stoi(readable_version);

  return toc_file;
}

}
