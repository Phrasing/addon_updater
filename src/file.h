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

ReadFileResult ReadFile(const char *path);

}  // namespace addon_updater

#endif  // !ADDON_UPDATER_FILE_H
