#ifndef ADDON_UPDATER_FILE_HANDLE_H
#define ADDON_UPDATER_FILE_HANDLE_H

namespace addon_updater {

class WindowsHandleFile {
 public:
  explicit WindowsHandleFile(HANDLE handle) noexcept;

  WindowsHandleFile(const WindowsHandleFile &) = delete;
  WindowsHandleFile &operator=(const WindowsHandleFile &) = delete;
  ~WindowsHandleFile();

  HANDLE Get() noexcept;

  std::optional<int> Read(void *buffer, int buffer_size) noexcept;

  std::optional<int> Write(const void *buffer, int buffer_size,
                           bool truncate) noexcept;

 private:
  HANDLE handle_;
};
}

#endif  // !ADDON_UPDATER_FILE_HANDLE_H
