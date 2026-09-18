#include "tracker/engine.hpp"
#include "tracker/parser.hpp"
#include "tracker/storage.hpp"

namespace tracker {

std::vector<Task> Engine::get_all_tasks() {
  std::vector<Task> tasks;
  std::vector<std::string> ids = Storage::get_task_folder_names();

  for (const std::string &id : ids) {
    std::optional<std::string> content = Storage::read_task_file(id);
    if (!content.has_value())
      continue;

    std::optional<Task> task = Parser::parse_task(content.value());
    if (task.has_value()) {
      task.value().id = id;
      tasks.push_back(task.value());
    }
  }

  return tasks;
}

std::optional<Task> Engine::get_task_by_id(const std::string &id) {
  std::optional<std::string> content = Storage::read_task_file(id);
  if (!content.has_value())
    return std::nullopt;

  std::optional<Task> task = Parser::parse_task(content.value());
  if (task.has_value()) {
    task.value().id = id;
  }
  return task;
}

std::vector<std::string> Engine::get_blocking_dependencies(const Task &task) {
  std::vector<std::string> blockers;
  for (const std::string &dep_id : task.depends_on) {
    std::optional<Task> dep_task = get_task_by_id(dep_id);
    if (!dep_task.has_value() || dep_task->status != Status::Closed) {
      blockers.push_back(dep_id);
    }
  }
  return blockers;
}

bool Engine::is_task_blocked(const Task &task) {
  return !get_blocking_dependencies(task).empty();
}

} // namespace tracker
