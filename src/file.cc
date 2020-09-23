// clang-format off
#include "pch.h"
#include "file.h"
#include "windows_error_message.h"
// clang-format on

namespace addon_updater {
namespace {

constexpr auto kBufferSize = 1024;

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

  std::optional<int> Write(const void *buffer, int buffer_size,
                           bool truncate) noexcept {
    DWORD write_size;

    if (truncate) {
      ::SetEndOfFile(this->handle_);
    }

    if (!::WriteFile(this->handle_, buffer, buffer_size, &write_size,
                     nullptr)) {
      return std::nullopt;
    }
    return static_cast<int>(write_size);
  }

 private:
  HANDLE handle_;
};

}  // namespace

bool WriteFileBuffered(WindowsHandleFile &file, std::string_view data,
                       bool truncate) {
  int total_bytes = 0;
  for (;;) {
    const auto bytes_written = file.Write(data.data() + total_bytes,
                                          data.size() - total_bytes, truncate);
    if (!bytes_written.has_value() || *bytes_written == 0) {
      std::fprintf(stderr, "Error: failed to write to file\n%s\n",
                   WindowsErrorMessage(GetLastError()).c_str());
      return false;
    }

    total_bytes += *bytes_written;

    if (static_cast<size_t>(*bytes_written) >= data.size()) break;
  }

  return (total_bytes > 0);
}

void ReadFileBuffered(WindowsHandleFile &file, int buffer_size,
                      ReadFileResult *out) {
  for (;;) {
    auto size_before = out->content.size();

    out->content.resize(size_before + buffer_size);

    std::optional<int> read_size =
        file.Read(&out->content[size_before], buffer_size);

    if (!read_size.has_value()) {
      out->error =
          "Failed to read from file: " + WindowsErrorMessage(::GetLastError());
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
    result.error =
        "Failed to read from file: " + WindowsErrorMessage(::GetLastError());
    return result;
  }
  result.content.resize(*read_size);
  if (*read_size >= size_to_read) {
    ReadFileBuffered(file, buffer_size, &result);
  }

  return result;
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

  assert(file_size.QuadPart <= std::numeric_limits<std::int64_t>::max());

  return ReadFileWithExpectedSize(file, static_cast<int>(file_size.QuadPart),
                                  kBufferSize);
}

bool WriteFile(std::string_view path, std::string_view buffer, bool truncate) {
  const auto handle =
      ::CreateFileA(path.data(), GENERIC_WRITE,
                    FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                    nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

  if (handle == INVALID_HANDLE_VALUE) {
    return false;
  }

  WindowsHandleFile file(handle);
  return WriteFileBuffered(file, buffer, truncate);
}

bool OsFileExists(std::string_view file_name) {
  const auto attrib = ::GetFileAttributesA(file_name.data());

  if (attrib == INVALID_FILE_ATTRIBUTES) return false;
  if (attrib & FILE_ATTRIBUTE_DIRECTORY) return false;

  return true;
}

bool OsDirectoryExists(std::string_view directory_name) {
  const auto attrib = ::GetFileAttributesA(directory_name.data());

  if (attrib == INVALID_FILE_ATTRIBUTES) return false;
  if (attrib & FILE_ATTRIBUTE_DIRECTORY) return true;

  return true;
}

ReadFileResult ReadFile(std::string_view path) {
  const auto handle =
      ::CreateFileA(path.data(), GENERIC_READ,
                    FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                    nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (handle == INVALID_HANDLE_VALUE) {
    const auto error = ::GetLastError();
    return ReadFileResult::Failure(std::string("failed to open ") +
                                   path.data() + ": " +
                                   WindowsErrorMessage(error));
  }
  WindowsHandleFile file(handle);
  return ReadEntireFile(path.data(), file);
}

std::optional<std::string> GetWindowsDriveLetterPrefix() {
  std::vector<char> buffer(MAX_PATH);
  auto result = ::GetWindowsDirectoryA(buffer.data(), buffer.size());
  buffer.resize(result);
  if (buffer.empty()) return {};
  return std::string(buffer.begin(), buffer.begin() + 3);
}

}  // namespace addon_updater
