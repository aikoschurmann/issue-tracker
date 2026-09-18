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
int CLI::handle_init(std::span<const char *> args) {
  std::filesystem::path root = std::filesystem::current_path();
  std::filesystem::path tasks_dir = root / "tasks";
  std::filesystem::path config_file = root / ".trackerconfig";

  bool created = false;
  if (!std::filesystem::exists(tasks_dir)) {
    std::filesystem::create_directories(tasks_dir);
    std::cout << colors::GREEN << "Created directory: " << colors::RESET
              << "tasks/\n";
    created = true;
  }

  if (!std::filesystem::exists(config_file)) {
    std::ofstream out(config_file);
    out << "";
    std::cout << colors::GREEN << "Created config: " << colors::RESET
              << ".trackerconfig\n";
    created = true;
  }

  std::filesystem::path gitignore = root / ".gitignore";
  if (std::filesystem::exists(gitignore)) {
    std::ifstream in(gitignore);
    std::string cnt((std::istreambuf_iterator<char>(in)),
                    std::istreambuf_iterator<char>());
    in.close();
    if (cnt.find(".trackerconfig") == std::string::npos) {
      std::ofstream out(gitignore, std::ios_base::app);
      out << "\n.trackerconfig\n";
      std::cout << colors::GREEN << "Updated file: " << colors::RESET
                << ".gitignore\n";
    }
  }
  Config::root_dir = root;

  if (!created)
    std::cout << "Tracker is already initialized in this directory.\n";
  else
    std::cout << "Tracker initialized successfully.\n";
  return 0;
}

int CLI::handle_config(std::span<const char *> args) {
  if (args.empty()) {
    std::cerr << "Usage: <command> config <key> [value]\n";
    return 1;
  }
  std::string key = args[0];
  if (args.size() > 1) {
    std::string value = args[1];
    Config::set(key, value);
    std::cout << colors::GREEN << "Set " << key << " = " << value
              << colors::RESET << "\n";
  } else {
    std::string val = Config::get(key);
    if (val.empty()) {
      std::cerr << colors::GRAY << "No value set for " << key << colors::RESET
                << "\n";
      return 1;
    }
    std::cout << val << "\n";
  }
  return 0;
}

int CLI::handle_log(std::span<const char *> args) {
  int count = 20;
  for (size_t i = 0; i < args.size(); ++i) {
    std::string_view a(args[i]);
    if (a == "-n" && i + 1 < args.size()) {
      count = std::stoi(std::string(args[i + 1]));
      i++;
    } else if (a == "--all") {
      count = -1;
    }
  }
  git_log(count);
  return 0;
}

int CLI::handle_help(std::span<const char *> args) {
  print_header("Issue Tracker CLI");
  std::cout << "Usage: <command> [args]\n\n";

  auto print_cmd = [](const char *cmd, const char *arg, const char *desc) {
    std::cout << "  " << colors::GREEN << std::left << std::setw(10) << cmd
              << colors::RESET << std::left << std::setw(20) << arg << desc
              << "\n";
  };

  std::cout << colors::BOLD << "Task Management\n" << colors::RESET;
  print_cmd("new", "<title>", "Create task (flags: -d, -p, -t, --deps, -e)");
  print_cmd("edit", "<id>", "Open a task in $EDITOR (or core.editor)");
  print_cmd("set", "<id> [flags]",
            "Set metadata (flags: -p/--priority, -t/--tags)");
  print_cmd("open", "<id>", "Mark a task as OPEN");
  print_cmd("close", "<id>", "Mark a task as CLOSED");
  print_cmd("rm", "<id> [-r]", "Delete a task (-r for recursive delete)");
  print_cmd("link", "<t> <d>", "Make <t> depend on <d>");
  print_cmd("unlink", "<t> <d>", "Remove dependency <d> from <t>");
  std::cout << "\n";

  std::cout << colors::BOLD << "Views & Visualizations\n" << colors::RESET;
  print_cmd("ls", "[-a|-c]",
            "List open tasks (include closed with -a/--all/-c/--closed)");
  print_cmd("tree", "[-a] [-d <n>]",
            "Visualize DAG (-a for closed, -d/--depth <n> to truncate)");
  print_cmd("requires", "<id>", "Visualize dependencies for a specific task");
  print_cmd("status", "", "View project health dashboard and progress");
  print_cmd("plan", "", "Topological sort of exactly what to do next");
  print_cmd("bottleneck", "", "Critical Path Analysis (find worst blockers)");
  print_cmd("burndown", "", "ASCII velocity chart (last 14 days)");
  std::cout << "\n";

  std::cout << colors::BOLD << "Config & System\n" << colors::RESET;
  print_cmd("config", "<key> [val]",
            "Get or set config (user.name, core.editor, ui.labels.unblocked)");
  print_cmd("log", "[-n <count>|--all]",
            "View history of background actions (default: 20)");
  print_cmd("undo", "[target]",
            "Undo the last action, or revert to a specific target");
  print_cmd("reset", "<target>",
            "Hard reset the task database to a specific point in time");
  std::cout << "\n";

  std::cout << colors::DIM
            << "Note: <id> arguments support Git-style prefix matching. You "
               "only need to type the first few letters of a Task ID."
            << colors::RESET << "\n\n";
  return 0;
}

void CLI::git_log(int limit) {
  std::filesystem::path git_dir = Config::root_dir / "tasks" / ".trackergit";
  std::filesystem::path tasks_dir = Config::root_dir / "tasks";
  if (!std::filesystem::exists(git_dir)) {
    std::cout << "No history available.\n";
    return;
  }

  print_header("Timeline");

  std::string cmd = "git --git-dir=" + escape_shell(git_dir.string()) +
                    " --work-tree=" + escape_shell(tasks_dir.string()) +
                    " log --format=\"%h|%cr|%s\"";
  if (limit > 0)
    cmd += " -n " + std::to_string(limit);
  FILE *pipe = popen(cmd.c_str(), "r");
  if (!pipe)
    return;

  char buffer[256];
  int count = 0;
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    std::string line(buffer);
    size_t first = line.find('|');
    size_t second = line.find('|', first + 1);
    if (first != std::string::npos && second != std::string::npos) {
      std::string hash = line.substr(0, first);
      std::string time = line.substr(first + 1, second - first - 1);
      std::string msg = line.substr(second + 1);

      if (!msg.empty() && msg.back() == '\n')
        msg.pop_back();

      // Highlight the very first action (current state) in green
      std::string hash_color = (count == 0) ? std::string(colors::GREEN)
                                            : std::string(colors::YELLOW);

      std::cout << hash_color << "[" << hash << "] " << colors::GRAY
                << std::setw(15) << std::left << time << colors::RESET << " "
                << msg << "\n";
      count++;
    }
  }
  pclose(pipe);
  std::cout << "\n";
}

void CLI::print_header(const std::string &title) {
  std::cout << "\n"
            << colors::BOLD << colors::CYAN << title << colors::RESET << "\n\n";
}

std::string CLI::get_target_task_id(std::span<const char *> args) {
  if (!args.empty()) {
    return resolve_task_id(std::string(args[0]));
  }
  std::optional<std::string> implicit_id = get_current_branch_task();
  if (implicit_id.has_value()) {
    return resolve_task_id(implicit_id.value());
  }
  return "";
}

} // namespace tracker
