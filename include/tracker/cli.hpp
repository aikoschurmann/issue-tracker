#pragma once
#include "tracker/task.hpp"
#include <span>
#include <string_view>

#include <unordered_set>

namespace tracker {

class CLI {
public:
  // Main entry point for the CLI router
  static int run(std::span<const char *> args);

private:
  // Command handlers
  static int handle_ls(std::span<const char *> args);
  static int handle_requires(std::span<const char *> args);
  static int handle_tree(std::span<const char *> args);
  static int handle_new(std::span<const char *> args);
  static int handle_close(std::span<const char *> args);
  static int handle_open(std::span<const char *> args);
  static int handle_rm(std::span<const char *> args);
  static int handle_edit(std::span<const char *> args);
  static int handle_status(std::span<const char *> args);
  static int handle_log(std::span<const char *> args);
  static int handle_plan(std::span<const char *> args);
  static int handle_link(std::span<const char *> args);
  static int handle_unlink(std::span<const char *> args);
  static int handle_bottleneck(std::span<const char *> args);
  static int handle_burndown(std::span<const char *> args);
  static int handle_init(std::span<const char *> args);
  static int handle_help(std::span<const char *> args);
  static std::string resolve_task_id(const std::string &prefix);
  static std::string get_target_task_id(std::span<const char *> args);
  static int handle_start(std::span<const char *> args);
  static int handle_finish(std::span<const char *> args);
  static int handle_submit(std::span<const char *> args);
  static int handle_config(std::span<const char *> args);
  static int handle_set(std::span<const char *> args);

  // LEVEL 3: Presentation & Utilities
  static void git_log(int limit = 20);

  static std::string create_task_file(const std::string &title,
                                      const std::string &desc, int priority,
                                      const std::string &tags,
                                      const std::string &depends);
  static void modify_status(const std::string &id,
                            const std::string &new_status);
  static void modify_dependencies(const std::string &id,
                                  const std::string &dep_id, bool add);
  static void recursive_rm(const std::string &id,
                           std::unordered_set<std::string> &deleted);

  static void print_header(const std::string &title);
  static void print_task(const Task &task);
  static void print_tree(const std::string &task_id, int depth,
                         std::vector<std::string> &seen,
                         std::vector<bool> &is_last,
                         std::unordered_set<std::string> &globally_seen,
                         bool show_closed = false, int max_depth = -1);
  static bool parse_ls_flags(std::span<const char *> args);
};

} // namespace tracker
