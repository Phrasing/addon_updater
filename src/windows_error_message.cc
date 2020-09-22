// clang-format off
#include "pch.h"
#include "windows_error_message.h"
// clang-format on

namespace addon_updater {
std::string WindowsErrorMessage(DWORD error_code) {
  LPSTR get_last_error_message;

  const auto get_last_error_message_length = ::FormatMessageA(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
          FORMAT_MESSAGE_IGNORE_INSERTS,
      nullptr, ::GetLastError(), 0,
      reinterpret_cast<LPSTR>(&get_last_error_message),
      (std::numeric_limits<DWORD>::max)(), nullptr);

  if (get_last_error_message_length == 0) {
    return "unknown error";
  }

  std::string_view message(
      get_last_error_message,
      static_cast<std::size_t>(get_last_error_message_length));

  message = string_util::RemoveSuffixIfPresent(message, "\r\n");
  std::string message_copy(message);
  static_cast<void>(::LocalFree(get_last_error_message));
  return message_copy;
}

void WindowsErrorMessageBox(std::string_view message) {
  static_cast<void>(
      ::MessageBoxA(nullptr, message.data(), nullptr, MB_ICONERROR));
}
}
