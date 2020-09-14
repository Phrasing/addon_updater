#ifndef ADDON_UPDATER_TOC_PARSER_H
#define ADDON_UPDATER_TOC_PARSER_H
#pragma once

namespace addon_updater {

struct TocFile {
  std::string title;
  std::string notes;
  std::string readable_version;
  int32_t numeric_version;
  std::string author;
  std::string interface;
};

std::optional<TocFile> ParseTocFile(std::string_view file_path);

}

#endif  // !
