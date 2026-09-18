#pragma once
#include <optional>
#include <string>
#include <vector>

namespace tracker {

class Storage {
public:
  // Scans the filesystem and returns all task folder names (IDs)
  static std::vector<std::string> get_task_folder_names();
  static std::optional<std::string> read_task_file(const std::string &id);
};

} // namespace tracker
