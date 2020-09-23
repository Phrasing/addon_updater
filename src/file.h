#ifndef ADDON_UPDATER_FILE_H
#define ADDON_UPDATER_FILE_H
#pragma once

namespace addon_updater {
struct ReadFileResult {
  std::string content;
  std::string error;

  bool Ok() const noexcept { return this->error.empty(); }
  static ReadFileResult Failure(const std::string &error);
};

std::optional<std::string> GetWindowsDriveLetterPrefix();

ReadFileResult ReadFile(std::string_view path);

bool WriteFile(std::string_view path, std::string_view buffer,
               bool truncate = true);

bool OsFileExists(std::string_view file_name);
bool OsDirectoryExists(std::string_view directory_name);


}  // namespace addon_updater

#endif  // !ADDON_UPDATER_FILE_H
