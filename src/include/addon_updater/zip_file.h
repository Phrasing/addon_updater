#ifndef ADDON_UPDATER_ZIP_FILE_H
#define ADDON_UPDATER_ZIP_FILE_H

namespace addon_updater {
enum { kInvalidZipInfo = -1 };

struct ZipInfo {
  ZipInfo() : file_index(kInvalidZipInfo), is_directory(false) {}
  uint32_t file_index;
  std::string file_name;
  bool is_directory;
  mz_zip_archive* archive;
};

class ZipFile {
 public:
  ZipFile();
  explicit ZipFile(const std::vector<uint8_t>& bytes);
  ~ZipFile();

  void Load(const std::vector<uint8_t>& bytes);

  bool Unzip(std::string_view path, std::vector<std::string>* install_paths);

  bool Good() const;
  void Reset();

  size_t Count() const;

  mz_zip_archive* GetArchive();

  bool ForEach(const std::function<bool(const ZipInfo&)>& cb);

  bool Read(const ZipInfo& file, std::vector<uint8_t>* buf);
  std::vector<uint8_t> Read(const ZipInfo& file);

 private:
  void StartRead();
  ZipInfo GetInfo(const uint32_t index);

 private:
  std::unique_ptr<mz_zip_archive> archive_;
  std::vector<uint8_t> buffer_;
  std::string filename_;
};
}

#endif  // !ADDON_UPDATER_ZIP_FILE_H
