#include "cli_helpers.hpp"
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

namespace tracker {
int CLI::handle_tree(std::span<const char *> args) {
  bool show_closed = false;
  int max_depth = -1;
  for (size_t i = 0; i < args.size(); ++i) {
    std::string_view a(args[i]);
    if (a == "-a" || a == "--all" || a == "-c" || a == "--closed")
      show_closed = true;
    else if ((a == "-d" || a == "--depth") && i + 1 < args.size()) {
      try {
        max_depth = std::stoi(std::string(args[++i]));
      } catch (...) {
      }
    }
  }

  std::unordered_map<std::string, Task> open_tasks;
  for (const std::string &id : Storage::get_task_folder_names()) {
    if (auto t = Engine::get_task_by_id(id)) {
      if (!show_closed && t->status == Status::Closed)
        continue;
      open_tasks[id] = *t;
    }
  }
  std::unordered_map<std::string, int> in_degree;
  for (const auto &[id, task] : open_tasks) {
    in_degree[id] = 0;
  }
  for (const auto &[id, task] : open_tasks) {
    for (const auto &dep : task.depends_on) {
      if (open_tasks.find(dep) != open_tasks.end()) {
        in_degree[dep]++;
      }
    }
  }

  std::vector<std::string> roots;
  for (const auto &[id, deg] : in_degree) {
    if (deg == 0 || deg > 1)
      roots.push_back(id);
  }

  auto reaches = [&](const std::string &start, const std::string &target) {
    if (start == target)
      return false;
    std::vector<std::string> stack = {start};
    std::unordered_set<std::string> vis;
    while (!stack.empty()) {
      std::string curr = stack.back();
      stack.pop_back();
      if (curr == target)
        return true;
      if (vis.count(curr))
        continue;
      vis.insert(curr);
      if (open_tasks.find(curr) != open_tasks.end()) {
        for (const auto &d : open_tasks.at(curr).depends_on) {
          stack.push_back(d);
        }
      }
    }
    return false;
  };

  std::sort(roots.begin(), roots.end(),
            [&](const std::string &a, const std::string &b) -> bool {
              bool a_reaches_b = reaches(a, b);
              bool b_reaches_a = reaches(b, a);

              if (a_reaches_b && !b_reaches_a)
                return false;
              if (b_reaches_a && !a_reaches_b)
                return true;

              int pa = open_tasks.at(a).priority;
              int pb = open_tasks.at(b).priority;
              if (pa != pb)
                return pa > pb;
              return a < b;
            });

  std::cout << "\n"
            << colors::BOLD << colors::CYAN << "Project Forest" << colors::RESET
            << "\n\n";

  std::vector<std::string> seen;
  std::vector<bool> is_last;
  std::unordered_set<std::string> globally_seen;

  for (const auto &root : roots) {
    print_tree(root, 0, seen, is_last, globally_seen, show_closed, max_depth);
    std::cout << "\n";
  }

  std::vector<std::string> cycle_nodes;
  for (const auto &[id, deg] : in_degree) {
    if (globally_seen.find(id) == globally_seen.end()) {
      cycle_nodes.push_back(id);
    }
  }
  if (!cycle_nodes.empty() && max_depth == -1) {
    std::cout << colors::RED << "[!] Orphaned Cycle Detected:" << colors::RESET
              << "\n";
    std::sort(cycle_nodes.begin(), cycle_nodes.end(),
              [&](const std::string &a, const std::string &b) -> bool {
                int pa = open_tasks.at(a).priority;
                int pb = open_tasks.at(b).priority;
                if (pa != pb)
                  return pa > pb;
                return a < b;
              });
    for (const auto &node : cycle_nodes) {
      if (globally_seen.find(node) == globally_seen.end()) {
        print_tree(node, 0, seen, is_last, globally_seen, show_closed,
                   max_depth);
        std::cout << "\n";
      }
    }
  }
  return 0;
}

bool CLI::parse_ls_flags(std::span<const char *> args) {
  for (const char *arg : args) {
    std::string_view s(arg);
    if (s == "-a" || s == "--all" || s == "-c" || s == "--closed")
      return true;
  }
  return false;
}

static void print_task_meta(const Task &task,
                            const std::string &prefix = "    ") {
  std::string date_str =
      task.created_at.empty() ? "2026-09-15" : task.created_at.substr(0, 10);
  std::string auth = task.author.empty() ? "unknown" : task.author;

  std::cout << prefix << colors::GRAY << "╰─ " << date_str << " · " << auth;
  if (!task.tags.empty()) {
    std::cout << colors::GRAY << " · [";
    for (size_t i = 0; i < task.tags.size(); ++i) {
      std::cout << task.tags[i] << (i + 1 < task.tags.size() ? "," : "");
    }
    std::cout << "]";
  }
  std::cout << colors::RESET << "\n";
}
void CLI::print_task(const Task &task) {
  std::optional<std::string> current = get_active_task();
  bool is_current = current.has_value() && current.value() == task.id;

  std::string p_str = "P" + std::to_string(task.priority);
  if (is_current) {
    std::string current_branch = get_current_git_branch();
    bool is_on_branch = (current_branch == "task/" + task.id);
    std::string tag = is_on_branch ? "[ACTIVE BRANCH]" : "[ACTIVE CONTEXT]";
    
    std::cout << colors::GREEN << "► " << colors::YELLOW
              << pad_truncate(p_str, 5) << colors::RESET;
    std::cout << colors::CYAN << task.id << colors::GREEN
              << " " << tag << colors::RESET << "\n";
  } else {
    std::cout << "  " << colors::YELLOW << pad_truncate(p_str, 5)
              << colors::RESET;
    std::cout << colors::CYAN << task.id << colors::RESET << "\n";
  }

  print_task_meta(task, "    ");
}

int CLI::handle_ls(std::span<const char *> args) {
  bool show_closed = parse_ls_flags(args);
  std::vector<Task> tasks = Engine::get_all_tasks();

  if (tasks.empty()) {
    std::cout << "No tasks found.\n";
    return 0;
  }

  std::sort(tasks.begin(), tasks.end(), [](const Task &a, const Task &b) {
    if (a.priority != b.priority)
      return a.priority > b.priority;
    return a.id < b.id;
  });

  std::unordered_map<std::string, Task> task_map;
  for (const Task &task : tasks) {
    task_map[task.id] = task;
  }

  std::vector<Task> blocked_tasks, open_tasks, closed_tasks;
  for (const Task &task : tasks) {
    if (task.status == Status::Closed) {
      if (show_closed)
        closed_tasks.push_back(task);
    } else {
      bool is_blocked = false;
      for (const std::string &dep_id : task.depends_on) {
        if (task_map.find(dep_id) == task_map.end() ||
            task_map[dep_id].status != Status::Closed) {
          is_blocked = true;
          break;
        }
      }
      if (is_blocked)
        blocked_tasks.push_back(task);
      else
        open_tasks.push_back(task);
    }
  }

  print_header(show_closed ? "All Tasks" : "Open Tasks");

  if (!blocked_tasks.empty()) {
    std::cout << colors::RED << "▼ BLOCKED" << colors::RESET << " ("
              << blocked_tasks.size() << ")\n";
    for (const Task &t : blocked_tasks)
      print_task(t);
    std::cout << "\n";
  }
  if (!open_tasks.empty()) {
    std::cout << colors::GREEN << "▼ OPEN" << colors::RESET << " ("
              << open_tasks.size() << ")\n";
    for (const Task &t : open_tasks)
      print_task(t);
    std::cout << "\n";
  }
  if (!closed_tasks.empty()) {
    std::cout << colors::GRAY << "▼ CLOSED" << colors::RESET << " ("
              << closed_tasks.size() << ")\n";
    for (const Task &t : closed_tasks)
      print_task(t);
    std::cout << "\n";
  }

  return 0;
}

int CLI::handle_status(std::span<const char *> args) {
  std::vector<Task> tasks = Engine::get_all_tasks();
  int total = tasks.size();
  int closed = 0;
  int blocked = 0;
  for (const Task &t : tasks) {
    if (t.status == Status::Closed)
      closed++;
    else if (Engine::is_task_blocked(t))
      blocked++;
  }
  int open_unblocked = total - closed - blocked;

  print_header("Project Status Dashboard");
  std::cout << "  " << colors::GRAY << "Total Tasks : " << colors::RESET
            << total << "\n";
  std::cout << "  " << colors::GREEN << "Closed Tasks: " << colors::RESET
            << closed;
  if (total > 0)
    std::cout << colors::GRAY << " (" << (closed * 100 / total) << "%)"
              << colors::RESET;
  std::cout << "\n";
  std::cout << "  " << colors::YELLOW << "Blocked     : " << colors::RESET
            << blocked << "\n";
  std::cout << "  " << colors::CYAN << "Open (Ready): " << colors::RESET
            << open_unblocked << "\n\n";

  return 0;
}

int CLI::handle_plan(std::span<const char *> args) {
  std::vector<Task> all_tasks = Engine::get_all_tasks();
  std::unordered_map<std::string, Task> open_tasks;
  std::unordered_map<std::string, int> in_degree;

  for (const Task &t : all_tasks) {
    if (t.status == Status::Open) {
      open_tasks[t.id] = t;
      in_degree[t.id] = 0;
    }
  }

  for (const auto &[id, t] : open_tasks) {
    for (const std::string &dep_id : t.depends_on) {
      if (open_tasks.count(dep_id))
        in_degree[dep_id]++;
    }
  }

  std::vector<std::string> roots;
  for (const auto &[id, deg] : in_degree) {
    if (deg == 0)
      roots.push_back(id);
  }

  std::sort(roots.begin(), roots.end(),
            [&](const std::string &a, const std::string &b) -> bool {
              return open_tasks[a].priority > open_tasks[b].priority;
            });

  print_header("Execution Plan");

  if (roots.empty() && open_tasks.empty()) {
    std::cout << "All tasks are closed! Nothing to plan.";
    return 0;
  }

  std::unordered_set<std::string> globally_planned, globally_simulated_closed;
  std::vector<std::string> isolated_roots;

  // Handle floating cycles that have no root
  while (globally_planned.size() < open_tasks.size()) {
    if (roots.empty()) {
      for (const auto &[id, t] : open_tasks) {
        if (!globally_planned.count(id)) {
          roots.push_back(id);
          break;
        }
      }
    }
    if (roots.empty())
      break;

    std::string root_id = roots.front();
    roots.erase(roots.begin());

    std::unordered_set<std::string> subgraph;
    std::vector<std::string> stack = {root_id};
    while (!stack.empty()) {
      std::string curr = stack.back();
      stack.pop_back();
      if (open_tasks.count(curr) && subgraph.insert(curr).second) {
        for (const std::string &dep : open_tasks[curr].depends_on)
          stack.push_back(dep);
      }
    }

    for (auto it = subgraph.begin(); it != subgraph.end();) {
      if (globally_planned.count(*it))
        it = subgraph.erase(it);
      else
        ++it;
    }
    if (subgraph.empty())
      continue;

    if (subgraph.size() == 1) {
      isolated_roots.push_back(root_id);
      globally_planned.insert(root_id);
      continue;
    }

    std::cout << colors::CYAN << "▼ " << root_id << colors::RESET << "\n";

    std::vector<std::string> plan;
    std::unordered_map<std::string, Task> local_open;
    for (const std::string &id : subgraph)
      local_open[id] = open_tasks[id];

    while (!local_open.empty()) {
      std::vector<std::string> ready_this_round;
      for (const auto &[id, t] : local_open) {
        bool is_ready = true;
        for (const std::string &dep_id : t.depends_on) {
          if (open_tasks.count(dep_id) &&
              !globally_simulated_closed.count(dep_id)) {
            is_ready = false;
            break;
          }
        }
        if (is_ready)
          ready_this_round.push_back(id);
      }

      if (ready_this_round.empty())
        break;

      std::sort(ready_this_round.begin(), ready_this_round.end(),
                [&](const std::string &a, const std::string &b) {
                  return local_open[a].priority > local_open[b].priority;
                });

      for (const std::string &id : ready_this_round) {
        plan.push_back(id);
        globally_simulated_closed.insert(id);
        globally_planned.insert(id);
        local_open.erase(id);
      }
    }

    for (const std::string &id : plan) {
      const Task &task = open_tasks[id];
      std::string p_str = "P" + std::to_string(task.priority);
      std::cout << "  " << colors::YELLOW << pad_truncate(p_str, 5)
                << colors::RESET;
      std::cout << colors::CYAN << task.id << colors::RESET;

      std::cout << "\n";

      std::string date_str = task.created_at.empty()
                                 ? "2026-09-15"
                                 : task.created_at.substr(0, 10);
      std::string auth = task.author.empty() ? "unknown" : task.author;

      std::cout << "    " << colors::GRAY << "╰─ " << date_str << " · " << auth;
      if (!task.tags.empty()) {
        std::cout << colors::GRAY << " · [";
        for (size_t i = 0; i < task.tags.size(); ++i)
          std::cout << task.tags[i] << (i + 1 < task.tags.size() ? "," : "");
        std::cout << "]";
      }
      std::cout << colors::RESET << "\n";
    }
    std::cout << "\n";
  }

  if (!isolated_roots.empty()) {
    std::cout << colors::CYAN << "▼ Standalone Tasks" << colors::RESET << "\n";
    std::sort(isolated_roots.begin(), isolated_roots.end(),
              [&](const std::string &a, const std::string &b) {
                return open_tasks[a].priority > open_tasks[b].priority;
              });
    for (const std::string &id : isolated_roots) {
      const Task &task = open_tasks[id];
      std::string p_str = "P" + std::to_string(task.priority);
      std::cout << "  " << colors::YELLOW << pad_truncate(p_str, 5)
                << colors::RESET;
      std::cout << colors::CYAN << task.id << colors::RESET;

      std::cout << "\n";

      std::string date_str = task.created_at.empty()
                                 ? "2026-09-15"
                                 : task.created_at.substr(0, 10);
      std::string auth = task.author.empty() ? "unknown" : task.author;

      std::cout << "    " << colors::GRAY << "╰─ " << date_str << " · " << auth;
      if (!task.tags.empty()) {
        std::cout << colors::GRAY << " · [";
        for (size_t i = 0; i < task.tags.size(); ++i)
          std::cout << task.tags[i] << (i + 1 < task.tags.size() ? "," : "");
        std::cout << "]";
      }
      std::cout << colors::RESET << "\n";
    }
    std::cout << "\n";
  }

  return 0;
}

int CLI::handle_requires(std::span<const char *> args) {
  if (args.empty()) {
    std::cerr << "Usage: issue-tracker requires <task_id>";
    return 1;
  }
  std::string task_id = std::string(args[0]);
  bool show_closed = parse_ls_flags(args);

  auto task = Engine::get_task_by_id(task_id);
  if (!task) {
    std::cerr << "Task not found.";
    return 1;
  }

  std::vector<std::string> seen;
  std::vector<bool> is_last;
  std::unordered_set<std::string> globally_seen;
  print_tree(task_id, 0, seen, is_last, globally_seen, show_closed, -1);
  return 0;
}

void CLI::print_tree(const std::string &task_id, int depth,
                     std::vector<std::string> &seen, std::vector<bool> &is_last,
                     std::unordered_set<std::string> &globally_seen,
                     bool show_closed, int max_depth) {
  std::string prefix = "";
  for (int i = 0; i < depth - 1; ++i)
    prefix += (is_last[i] ? "    " : "│   ");
  if (depth > 0)
    prefix += (is_last.back() ? "└── " : "├── ");

  if (std::find(seen.begin(), seen.end(), task_id) != seen.end()) {
    std::cout << colors::GRAY << prefix << colors::RESET << colors::CYAN
              << task_id << colors::RESET << " " << colors::GRAY
              << "[CYCLE DETECTED]" << colors::RESET << "\n";
    return;
  }
  seen.push_back(task_id);

  if (globally_seen.find(task_id) != globally_seen.end()) {
    std::cout << colors::GRAY << prefix << colors::RESET << colors::CYAN
              << task_id << colors::RESET << " " << colors::GRAY << "[SHARED]"
              << colors::RESET << "\n";
    seen.pop_back();
    return;
  }
  globally_seen.insert(task_id);

  auto task_opt = Engine::get_task_by_id(task_id);
  if (!task_opt) {
    std::cout << colors::GRAY << prefix << colors::RESET << colors::CYAN
              << task_id << colors::RESET << " " << colors::RED << "[MISSING]"
              << colors::RESET << "\n";
    seen.pop_back();
    return;
  }

  Task task = task_opt.value();

  std::vector<std::string> visible_deps;
  for (const auto &dep : task.depends_on) {
    auto opt = Engine::get_task_by_id(dep);
    if (opt) {
      if (show_closed || opt->status != Status::Closed) {
        visible_deps.push_back(dep);
      }
    }
  }

  std::cout << colors::GRAY << prefix << colors::RESET;

  std::cout << colors::CYAN << task.id << colors::RESET;
  if (task.status == Status::Closed)
    std::cout << " " << colors::GRAY << "[CLOSED]" << colors::RESET;
  else if (Engine::is_task_blocked(task))
    std::cout << " " << colors::RED << "[BLOCKED]" << colors::RESET;
  else
    std::cout << " " << colors::GREEN << "[OPEN]" << colors::RESET;
  std::cout << "\n";

  std::string meta_prefix = "";
  for (int i = 0; i < depth; ++i)
    meta_prefix += (is_last[i] ? "    " : "│   ");

  bool truly_has_children = !visible_deps.empty();
  bool will_draw_children =
      truly_has_children && (max_depth == -1 || depth < max_depth);
  if (will_draw_children)
    meta_prefix += "│   ";
  else
    meta_prefix += "    ";

  std::string c_at =
      task.created_at.empty() ? "2026-09-15" : task.created_at.substr(0, 10);
  std::string auth = task.author.empty() ? "unknown" : task.author;

  std::cout << colors::GRAY << meta_prefix << colors::RESET << colors::YELLOW
            << "P" << task.priority << colors::GRAY << " · " << c_at << " · "
            << auth;
  if (!task.tags.empty()) {
    std::cout << colors::GRAY << " · [";
    for (size_t i = 0; i < task.tags.size(); ++i)
      std::cout << task.tags[i] << (i + 1 < task.tags.size() ? "," : "");
    std::cout << "]";
  }
  std::cout << colors::RESET << "\n";

  if (max_depth == -1 || depth < max_depth) {
    for (size_t i = 0; i < visible_deps.size(); ++i) {
      is_last.push_back(i == visible_deps.size() - 1);
      print_tree(visible_deps[i], depth + 1, seen, is_last, globally_seen,
                 show_closed, max_depth);
      is_last.pop_back();
    }
  }
  seen.pop_back();
}
int CLI::handle_bottleneck(std::span<const char *> args) {
  if (Config::find_root() == RootState::NotFound) {
    std::cerr << "Not in an IssueTracker repository.";
    return 1;
  }

  auto all_tasks = Engine::get_all_tasks();
  std::unordered_map<std::string, std::vector<std::string>> blocked_by;
  std::vector<std::string> open_task_ids;

  std::unordered_map<std::string, Task> tasks_map;
  for (const auto &task : all_tasks) {
    tasks_map[task.id] = task;
  }
  for (const auto &task : all_tasks) {
    if (task.status != Status::Closed) {
      open_task_ids.push_back(task.id);
      for (const auto &dep : task.depends_on) {
        if (tasks_map.count(dep) &&
            tasks_map.at(dep).status != Status::Closed) {
          blocked_by[dep].push_back(task.id);
        }
      }
    }
  }

  struct Bottleneck {
    std::string id;
    int downstream_count;
  };
  std::vector<Bottleneck> bottlenecks;

  for (const auto &id : open_task_ids) {
    std::unordered_set<std::string> reachable;
    std::vector<std::string> stack = {id};
    while (!stack.empty()) {
      std::string curr = stack.back();
      stack.pop_back();
      for (const auto &next : blocked_by[curr]) {
        if (reachable.insert(next).second) {
          stack.push_back(next);
        }
      }
    }
    if (reachable.size() > 0) {
      bottlenecks.push_back({id, (int)reachable.size()});
    }
  }

  std::sort(bottlenecks.begin(), bottlenecks.end(),
            [](const Bottleneck &a, const Bottleneck &b) {
              return a.downstream_count > b.downstream_count;
            });

  print_header("Critical Path Analysis");
  if (bottlenecks.empty()) {
    std::cout << colors::GRAY
              << "No bottlenecks found. Your tasks are unlinked or you have no "
                 "open tasks.\n"
              << colors::RESET << "\n";
    return 0;
  }

  for (size_t i = 0; i < std::min(bottlenecks.size(), (size_t)10); ++i) {
    auto &b = bottlenecks[i];
    auto &t = tasks_map.at(b.id);
    std::cout << colors::YELLOW << "[" << b.downstream_count << " downstream]  "
              << colors::RESET << colors::CYAN << pad_truncate(b.id, 42)
              << colors::RESET << "\n";
  }

  return 0;
}

int CLI::handle_burndown(std::span<const char *> args) {
  if (Config::find_root() == RootState::NotFound) {
    std::cerr << "Not in an IssueTracker repository.";
    return 1;
  }

  std::filesystem::path git_dir = Config::root_dir / "tasks" / ".trackergit";
  if (!std::filesystem::exists(git_dir)) {
    std::cout << "No history available.\n";
    return 0;
  }

  std::string cmd = "git --git-dir=" + escape_shell(git_dir.string()) +
                    " log --format=\"%ct|%s\"";
  FILE *pipe = popen(cmd.c_str(), "r");
  if (!pipe)
    return 1;

  std::time_t now = std::time(nullptr);
  std::vector<int> counts(14, 0);

  char buffer[256];
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    std::string line(buffer);
    size_t sep = line.find('|');
    if (sep != std::string::npos) {
      std::string ts_str = line.substr(0, sep);
      std::string msg = line.substr(sep + 1);
      if (msg.find("Close task") == 0) {
        try {
          std::time_t ts = std::stoll(ts_str);
          int days_ago = (now - ts) / (60 * 60 * 24);
          if (days_ago >= 0 && days_ago < 14) {
            counts[13 - days_ago]++;
          }
        } catch (...) {
        }
      }
    }
  }
  pclose(pipe);

  print_header("Burndown (Tasks Closed - Last 14 Days)");

  int max_val = 1;
  for (int c : counts)
    if (c > max_val)
      max_val = c;

  for (int i = 0; i < 14; ++i) {
    int days_ago = 13 - i;
    std::string label =
        days_ago == 0 ? "Today" : std::to_string(days_ago) + "d ago";
    std::cout << colors::GRAY << std::setw(8) << std::right << label << " "
              << colors::RESET << "| ";

    int bar_len = (counts[i] * 40) / max_val;
    if (counts[i] > 0)
      bar_len = std::max(1, bar_len);

    std::cout << colors::CYAN;
    for (int b = 0; b < bar_len; ++b)
      std::cout << "█";
    std::cout << colors::RESET;

    if (counts[i] > 0)
      std::cout << " " << counts[i];
    std::cout << "\n";
  }
  std::cout << "\n";

  return 0;
}
} // namespace tracker
