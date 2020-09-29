#ifndef ADDON_UPDATER_FILE_H
#define ADDON_UPDATER_FILE_H
#pragma once

namespace addon_updater {

struct ReadFileResult {
  std::string content;
  std::string error;

  bool Ok() const noexcept { return this->error.empty(); }
  static ReadFileResult Failure(const std::string& error);
};

struct DirectoryResult {
  std::string directory;
  std::string path;
  std::string parent_path;
};
using DirectoryCallback =
    std::function<bool(const DirectoryResult& directory_result)>;

bool IterateDirectory(std::string_view directory,
                      const DirectoryCallback& directory_callback);

std::optional<std::string> GetWindowsDriveLetterPrefix();
ReadFileResult ReadFile(const char* file_path);

bool WriteFile(const char* file_path, std::string_view buffer,
               bool truncate = true);
bool OsFileExists(const char* file);
bool OsDirectoryExists(const char* directory);

}  // namespace addon_updater

#endif  // !ADDON_UPDATER_FILE_H
