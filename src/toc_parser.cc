// clang-format off
#include "pch.h"
#include "file.h"
#include "toc_parser.h"
// clang-format on

namespace addon_updater {
TocParser::TocParser(std::string_view file_path)
    : new_line_regex_(boost::regex(R"(\|r)")),
      color_encoded_regex_(boost::regex(R"(\|[a-zA-Z0-9]{9})")) {
  this->Load(file_path);
}

TocParser::~TocParser() {}

void TocParser::Load(std::string_view file_path) noexcept {
  auto result = ReadFile(file_path.data());
  if (result.Ok()) {
    contents_ = std::move(result.content);
  }
}

std::optional<TocFile> TocParser::ParseTocFile() {
  TocFile toc_file{};
  toc_file.author = std::move(this->GetValue("Author"));
  toc_file.title = std::move(this->GetValue("Title"));
  toc_file.notes = std::move(this->GetValue("Notes"));

  toc_file.readable_version = std::move(this->GetValue("Version"));
  toc_file.stripped_version =
      string_util::StripNonDigits(toc_file.readable_version);

  toc_file.numeric_version =
      string_util::StringToNumber(toc_file.stripped_version);

  return toc_file;
}

std::string TocParser::GetValue(std::string_view key) {
  boost::match_results<std::string::const_iterator> results;
  if (boost::regex_search(
          static_cast<std::string::const_iterator>(contents_.begin()),
          static_cast<std::string::const_iterator>(contents_.end()), results,
          boost::regex("^## " + std::string(key) + ":(.*?)$"),
          boost::match_default)) {
    auto result = std::string(results[1].first, results[1].second);
    this->StripColorCharacters(result);
    this->StripNewlineCharacters(result);
    if (result.at(0) == ' ') {
      result.erase(0, 1);
    }
    return result;
  }
  return std::string();
}

void TocParser::StripNewlineCharacters(std::string& string) {
  string = boost::regex_replace(string, new_line_regex_, "");
}

void TocParser::StripColorCharacters(std::string& string) {
  string = boost::regex_replace(string, color_encoded_regex_, "");
}

}
