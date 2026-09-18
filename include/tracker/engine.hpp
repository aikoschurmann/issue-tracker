#pragma once
#include "tracker/task.hpp"
#include <optional>
#include <string>
#include <vector>

namespace tracker {

class Engine {
public:
  // Reads the database and fully parses all tasks
  static std::vector<Task> get_all_tasks();

  // Fetch a single task by its ID
  static std::optional<Task> get_task_by_id(const std::string &id);

  // Check if a task is blocked by any OPEN dependencies
  static bool is_task_blocked(const Task &task);
  static std::vector<std::string> get_blocking_dependencies(const Task &task);
};

} // namespace tracker
