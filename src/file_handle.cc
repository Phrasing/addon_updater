// clang-format off
#include <addon_updater/pch.h>
#include <addon_updater/file_handle.h>
// clang-format on

namespace addon_updater {
std::optional<int> WindowsHandleFile::Read(void* buffer,
                                           int buffer_size) noexcept {
  DWORD read_size;
  if (!::ReadFile(this->handle_, buffer, static_cast<DWORD>(buffer_size),
                  &read_size, nullptr)) {
    return std::nullopt;
  }
  return static_cast<int>(read_size);
}

std::optional<int> WindowsHandleFile::Write(const void* buffer, int buffer_size,
                                            bool truncate) noexcept {
  DWORD write_size;

  if (truncate) ::SetEndOfFile(this->handle_);
  if (!::WriteFile(this->handle_, buffer, buffer_size, &write_size, nullptr)) {
    return std::nullopt;
  }
  return static_cast<int>(write_size);
}

HANDLE WindowsHandleFile::Get() noexcept { return this->handle_; }

WindowsHandleFile::WindowsHandleFile(HANDLE handle) noexcept
    : handle_(handle){};

WindowsHandleFile::~WindowsHandleFile() {
  if (!::CloseHandle(this->handle_)) {
    std::fprintf(stderr, "Error: failed to close file %p\n", &handle_);
  }
}
}