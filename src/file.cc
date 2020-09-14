// clang-format off
#include "pch.h"
#include "file.h"
// clang-format on

namespace addon_updater {
namespace {

constexpr auto kBufferSize = 1024;

std::string_view RemoveSuffixIfPresent(std::string_view s,
                                       std::string_view suffix) {
  if (s.ends_with(suffix)) {
    s.remove_suffix(suffix.size());
  }
  return s;
}

std::string WindowsErrorMessage(DWORD error) {
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

  message = RemoveSuffixIfPresent(message, "\r\n");
  std::string message_copy(message);
  static_cast<void>(::LocalFree(get_last_error_message));
  return message_copy;
}

class WindowsHandleFile {
 public:
  explicit WindowsHandleFile(HANDLE handle) noexcept : handle_(handle) {}

  WindowsHandleFile(const WindowsHandleFile &) = delete;
  WindowsHandleFile &operator=(const WindowsHandleFile &) = delete;

  ~WindowsHandleFile() {
    if (!::CloseHandle(this->handle_)) {
      std::fprintf(stderr, "Error: failed to close file\n");
    }
  }

  HANDLE Get() noexcept { return this->handle_; }

  std::optional<int> Read(void *buffer, int buffer_size) noexcept {
    DWORD read_size;
    if (!::ReadFile(this->handle_, buffer, static_cast<DWORD>(buffer_size),
                    &read_size, nullptr)) {
      return std::nullopt;
    }
    return static_cast<int>(read_size);
  }

  static std::string get_last_error_message() {
    return WindowsErrorMessage(::GetLastError());
  }

 private:
  HANDLE handle_;
};

}  // namespace

void ReadFileBuffered(WindowsHandleFile &file, int buffer_size,
                      ReadFileResult *out) {
  for (;;) {
    auto size_before = out->content.size();

    out->content.resize(size_before + buffer_size);

    std::optional<int> read_size =
        file.Read(&out->content[size_before], buffer_size);
    if (!read_size.has_value()) {
      out->error = "Failed to read from file: " +
                   WindowsHandleFile::get_last_error_message();
      return;
    }
    out->content.resize(size_before + *read_size);
    if (*read_size < buffer_size) {
      return;
    }
  }
}

ReadFileResult ReadFileWithExpectedSize(WindowsHandleFile &file, int file_size,
                                        int buffer_size) {
  ReadFileResult result;
  const auto size_to_read = file_size + 1;
  result.content.resize(size_to_read);
  const auto read_size = file.Read(result.content.data(), size_to_read);
  if (!read_size.has_value()) {
    result.error = "Failed to read from file: " +
                   WindowsHandleFile::get_last_error_message();
    return result;
  }
  result.content.resize(*read_size);
  if (*read_size < size_to_read) {
    return result;
  } else {
    ReadFileBuffered(file, buffer_size, &result);
    return result;
  }
}

ReadFileResult ReadFileResult::Failure(const std::string &error) {
  ReadFileResult result;
  result.error = error;
  return result;
}

ReadFileResult ReadEntireFile(const char *path, WindowsHandleFile &file) {
  ::LARGE_INTEGER file_size;
  if (!::GetFileSizeEx(file.Get(), &file_size)) {
    const auto error = ::GetLastError();
    return ReadFileResult::Failure(std::string("failed to get size of file ") +
                                   path + ": " + WindowsErrorMessage(error));
  }
  return ReadFileWithExpectedSize(file, static_cast<int>(file_size.QuadPart),
                                  kBufferSize);
}

ReadFileResult ReadFile(const char *path) {
  auto *const handle =
      ::CreateFileA(path, GENERIC_READ,
                    FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                    nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (handle == INVALID_HANDLE_VALUE) {
    const auto error = ::GetLastError();
    return ReadFileResult::Failure(std::string("failed to open ") + path +
                                   ": " + WindowsErrorMessage(error));
  }
  WindowsHandleFile file(handle);
  return ReadEntireFile(path, file);
}

}  // namespace addon_updater
