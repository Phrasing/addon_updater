#ifndef ADDON_UPDATER_TOC_PARSER_H
#define ADDON_UPDATER_TOC_PARSER_H
#pragma once

namespace addon_updater {

struct TocFile {
  std::string title;
  std::string notes;
  std::string readable_version;
  size_t numeric_version;
  std::string stripped_version;
  size_t x_curse_project_id;
  std::string author;
  std::string interface;
};

class TocParser {
 public:
  explicit TocParser(std::string_view file_path);
  ~TocParser();

  TocParser(const TocParser&) = delete;
  TocParser& operator=(const TocParser&) = delete;

  bool Ok() const { return !contents_.empty(); }

  void Load(std::string_view file_path) noexcept;

  std::optional<TocFile> ParseTocFile();

 private:
  std::string GetValue(std::string_view key);

  void StripNewlineCharacters(std::string& string);
  void StripColorCharacters(std::string& string);

  std::string contents_;
  boost::regex new_line_regex_;
  boost::regex color_encoded_regex_;
};

}

#endif  // !
