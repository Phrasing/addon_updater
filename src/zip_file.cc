// clang-format off
#include "pch.h"
#include "file.h"
#include "zip_file.h"
// clang-format on

namespace addon_updater {

ZipFile::ZipFile() : archive_(new mz_zip_archive()) { Reset(); }

ZipFile::ZipFile(const std::vector<uint8_t>& bytes) : ZipFile() { Load(bytes); }

ZipFile::~ZipFile() { Reset(); }

bool ZipFile::Unzip(std::string_view path,
                    std::vector<std::string>* install_paths) {
  auto path_copy = std::string{path.data(), path.length()};

  if (path_copy.empty()) {
    return false;
  }


  if (path_copy.back() != '\\') path_copy += '\\';

  this->ForEach([&](const ZipInfo& zip_info) -> bool {
    if (zip_info.is_directory) return true;

    std::string full_path = path_copy + zip_info.file_name;

    std::string addon_path =
        path_copy +
        zip_info.file_name.substr(0, zip_info.file_name.find_first_of("\\/"));

    std::string relative_path =
        full_path.substr(0, full_path.find_last_of("\\/"));

    string_util::ReplaceAll(&full_path, R"(/)", R"(\)");
    string_util::ReplaceAll(&addon_path, R"(/)", R"(\)");
    string_util::ReplaceAll(&relative_path, R"(/)", R"(\)");

    std::remove(full_path.c_str());

    if (!OsDirectoryExists(relative_path.c_str())) {
      std::filesystem::create_directories(relative_path);
    }

    install_paths->push_back(addon_path);

    mz_zip_reader_extract_to_file(std::move(this->GetArchive()),
                                  zip_info.file_index, full_path.c_str(), 0);

    return true;
  });

  return true;
}

void ZipFile::Load(const std::vector<uint8_t>& bytes) {
  Reset();
  buffer_.assign(bytes.begin(), bytes.end());
  StartRead();
}

bool ZipFile::Good() const {
  return archive_->m_zip_mode != MZ_ZIP_MODE_INVALID;
}

void ZipFile::StartRead() {
  if (archive_->m_zip_mode == MZ_ZIP_MODE_READING) {
    return;
  }

  if (!mz_zip_reader_init_mem(archive_.get(), buffer_.data(), buffer_.size(),
                              0)) {
    Reset();
  }
}

void ZipFile::Reset() {
  if (archive_->m_zip_mode == MZ_ZIP_MODE_READING) {
    mz_zip_reader_end(archive_.get());
  }

  buffer_.clear();
  buffer_.shrink_to_fit();

  archive_->m_zip_mode = MZ_ZIP_MODE_INVALID;
}

size_t ZipFile::Count() const {
  return mz_zip_reader_get_num_files(archive_.get());
}

mz_zip_archive* ZipFile::GetArchive() { return archive_.get(); }

bool ZipFile::ForEach(const std::function<bool(const ZipInfo&)>& cb) {
  for (size_t iii = 0; iii < Count(); iii++) {
    const auto info = GetInfo(iii);
    if (info.file_index == kInvalidZipInfo) {
      return false;
    }
    if (!cb(info)) {
      break;
    }
  }
  return true;
}

bool ZipFile::Read(const ZipInfo& file, std::vector<uint8_t>* buf) {
  if (buf && !file.is_directory && file.file_index != kInvalidZipInfo) {
    size_t size;
    auto data = static_cast<uint8_t*>(mz_zip_reader_extract_to_heap(
        archive_.get(), static_cast<mz_uint>(file.file_index), &size, 0));
    if (data) {
      if (buf->capacity() < size) {
        buf->reserve(size);
      }
      std::copy(data, data + size, std::back_inserter(*buf));
      mz_free(data);
      return true;
    }
  }
  return false;
}

std::vector<uint8_t> ZipFile::Read(const ZipInfo& file) {
  std::vector<uint8_t> buf;
  Read(file, &buf);
  return buf;
}

ZipInfo ZipFile::GetInfo(const uint32_t index) {
  mz_zip_archive_file_stat stat;
  if (!mz_zip_reader_file_stat(archive_.get(), static_cast<mz_uint>(index),
                               &stat)) {
    return {};
  }

  ZipInfo zip_info;
  zip_info.file_index = stat.m_file_index;
  zip_info.file_name = stat.m_filename;
  zip_info.archive = archive_.get();
  zip_info.is_directory =
      mz_zip_reader_is_file_a_directory(archive_.get(), stat.m_file_index);
  return zip_info;
}
}