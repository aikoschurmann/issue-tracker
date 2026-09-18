#include "tracker/storage.hpp"
#include "tracker/config.hpp"
#include <filesystem>
#include <fstream>

namespace tracker {

std::vector<std::string> Storage::get_task_folder_names() {
  std::vector<std::string> task_ids;
  std::filesystem::path tasks_dir = tracker::Config::root_dir / "tasks";

  if (!std::filesystem::exists(tasks_dir)) {
    return task_ids;
  }

  for (const std::filesystem::directory_entry &entry :
       std::filesystem::directory_iterator(tasks_dir)) {
    if (entry.is_directory()) {
      std::string filename = entry.path().filename().string();
      if (filename.starts_with("."))
        continue;
      task_ids.push_back(filename);
    }
  }

  return task_ids;
}

std::optional<std::string> Storage::read_task_file(const std::string &id) {
  std::filesystem::path file_path =
      tracker::Config::root_dir / "tasks" / id / "TASK.md";

  if (!std::filesystem::exists(file_path)) {
    return std::nullopt;
  }

  std::ifstream file(file_path, std::ios::in | std::ios::binary);
  if (!file) {
    return std::nullopt;
  }

  // Fast way to read an entire file into a std::string in C++
  std::string content;
  file.seekg(0, std::ios::end);
  std::streamsize size = file.tellg();
  if (size == -1)
    return std::nullopt;
  content.resize(size);
  file.seekg(0, std::ios::beg);
  file.read(&content[0], content.size());

  return content;
}

} // namespace tracker
