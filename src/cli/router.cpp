#include "tracker/cli.hpp"
#include "tracker/colors.hpp"
#include "tracker/config.hpp"
#include "tracker/engine.hpp"
#include "tracker/storage.hpp"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <unordered_set>

#include "cli_helpers.hpp"

namespace tracker {
int CLI::run(std::span<const char *> args) {
  RootState state = Config::find_root();
  Config::load();

  if (!args.empty()) {
    std::string_view command = args[0];
    if ((command == "new" || command == "start") &&
        state == RootState::GitFallback) {
      std::cout << colors::GRAY
                << "No tracker root found -- using git repo root ("
                << Config::root_dir.string() << ")\n";
      std::cout << "Run 'issue-tracker init' to formalize this project.\n"
                << colors::RESET;
    }
  }
  if (args.empty()) {
    std::vector<const char *> empty_args;
    return handle_help(std::span<const char *>(empty_args));
  }

  std::string_view command = args[0];
  std::span<const char *> command_args = args.subspan(1);

  if (command == "ls")
    return handle_ls(command_args);
  if (command == "requires")
    return handle_requires(command_args);
  if (command == "tree")
    return handle_tree(command_args);
  if (command == "init")
    return handle_init(command_args);
  if (command == "config")
    return handle_config(command_args);
  if (command == "set")
    return handle_set(command_args);
  if (command == "new")
    return handle_new(command_args);
  if (command == "close")
    return handle_close(command_args);
  if (command == "open")
    return handle_open(command_args);
  if (command == "rm")
    return handle_rm(command_args);
  if (command == "edit")
    return handle_edit(command_args);
  if (command == "start")
    return handle_start(command_args);
  if (command == "finish")
    return handle_finish(command_args);
  if (command == "status")
    return handle_status(command_args);
  if (command == "log")
    return handle_log(command_args);
  if (command == "plan")
    return handle_plan(command_args);
  if (command == "link")
    return handle_link(command_args);
  if (command == "unlink")
    return handle_unlink(command_args);
  if (command == "bottleneck")
    return handle_bottleneck(command_args);
  if (command == "burndown")
    return handle_burndown(command_args);
  if (command == "_autocomplete") {
    for (const auto &f : Storage::get_task_folder_names())
      std::cout << f << "\n";
    return 0;
  }
  if (command == "help" || command == "--help" || command == "-h")
    return handle_help(command_args);

  std::cerr << "Unknown command: " << command << "\n";
  std::cerr << "Run 'help' to see available commands.\n";
  return 1;
}

std::string CLI::resolve_task_id(const std::string &prefix) {
  auto folders = Storage::get_task_folder_names();
  for (const auto &f : folders) {
    if (f == prefix)
      return prefix;
  }
  std::vector<std::string> matches;
  for (const auto &f : folders) {
    if (f.find(prefix) == 0) { // starts_with
      matches.push_back(f);
    }
  }
  if (matches.empty()) {
    std::cerr << colors::RED << "Error: " << colors::RESET
              << "No task found matching '" << prefix << "'\n";
    return "";
  }
  if (matches.size() > 1) {
    std::cerr << colors::RED << "Error: " << colors::RESET
              << "Ambiguous task ID '" << prefix << "'. Matches:\n";
    for (const auto &m : matches)
      std::cerr << "  " << m << "\n";
    return "";
  }
  return matches[0];
}

} // namespace tracker
