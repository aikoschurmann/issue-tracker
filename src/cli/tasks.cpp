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
#include <random>
#include <sstream>
#include <unordered_set>

#include "cli_helpers.hpp"

namespace tracker {
int CLI::handle_new(std::span<const char *> args) {
  std::string title;
  std::string desc;
  int priority = 100;
  std::string tags;
  std::string deps;
  bool open_editor = true;

  for (size_t i = 0; i < args.size(); ++i) {
    std::string arg = args[i];
    if (arg == "--desc" || arg == "-d") {
      if (i + 1 < args.size())
        desc = args[++i];
    } else if (arg == "--priority" || arg == "-p") {
      if (i + 1 < args.size()) {
        try {
          priority = std::stoi(args[++i]);
        } catch (...) {
        }
      }
    } else if (arg == "--tags" || arg == "-t") {
      if (i + 1 < args.size())
        tags = args[++i];
    } else if (arg == "--deps") {
      if (i + 1 < args.size())
        deps = args[++i];
    } else if (arg == "--edit" || arg == "-e") {
      open_editor = true;
    } else {
      if (!title.empty())
        title += " ";
      title += arg;
    }
  }

  if (title.empty()) {
    std::cerr << "Usage: <command> new <title> [-d desc] [-p priority] [-t "
                 "tags] [--deps deps] [-e]\n";
    return 1;
  }

  std::string id = create_task_file(title, desc, priority, tags, deps);
  std::cout << colors::GREEN << "Created task: " << colors::RESET << id << "\n";

  if (open_editor) {
    std::string editor = Config::get_global("core.editor", "");
    if (editor.empty()) {
      const char *e = std::getenv("EDITOR");
      editor = e ? e : "code";
    }
    std::string path = (Config::root_dir / "tasks" / id / "TASK.md").string();
    std::string cmd = editor + " " + escape_shell(path);
    std::system(cmd.c_str());
  }

  std::cout << "\n";
  std::vector<const char *> empty_args;
  handle_tree(std::span<const char *>(empty_args));
  return 0;
}

int CLI::handle_edit(std::span<const char *> args) {
  std::string id = get_target_task_id(args);
  if (id.empty()) {
    std::cerr
        << "Usage: <command> edit [<id>] (or run inside a task/ branch)\n";
    return 1;
  }
  std::filesystem::path path = Config::root_dir / "tasks" / id / "TASK.md";

  if (!std::filesystem::exists(path)) {
    std::cerr << "Task not found: " << id << "\n";
    return 1;
  }

  std::string editor = Config::get_global("core.editor", "");
  if (editor.empty()) {
    const char *e = std::getenv("EDITOR");
    editor = e ? e : "code";
  }

  std::string cmd = editor + " " + escape_shell(path.string());
  if (std::system(cmd.c_str()) == 0) {
    std::cout << colors::GREEN << "Saved task: " << colors::RESET << id << "\n";
  }
  return 0;
}

int CLI::handle_close(std::span<const char *> args) {
  std::string id = get_target_task_id(args);
  if (id.empty()) {
    std::cerr
        << "Usage: <command> close [<id>] (or run inside a task/ branch)\n";
    return 1;
  }
  modify_status(id, "CLOSED");
  std::cout << colors::GRAY << "Closed task: " << colors::RESET << id << "\n";
  return 0;
}

int CLI::handle_open(std::span<const char *> args) {
  std::string id = get_target_task_id(args);
  if (id.empty()) {
    std::cerr
        << "Usage: <command> open [<id>] (or run inside a task/ branch)\n";
    return 1;
  }
  modify_status(id, "OPEN");
  std::cout << colors::GREEN << "Opened task: " << colors::RESET << id << "\n";
  return 0;
}

int CLI::handle_rm(std::span<const char *> args) {
  if (args.empty())
    return 1;
  std::string target_id = resolve_task_id(std::string(args[0]));
  if (target_id.empty())
    return 1;
  bool recursive =
      (args.size() > 1 && (std::string_view(args[1]) == "-r" ||
                           std::string_view(args[1]) == "--recursive"));

  if (recursive) {
    std::unordered_set<std::string> deleted;
    recursive_rm(target_id, deleted);
  } else {
    std::filesystem::path dir = Config::root_dir / "tasks" / target_id;
    if (std::filesystem::exists(dir)) {
      std::filesystem::remove_all(dir);
      std::cout << colors::RED << "Deleted: " << colors::RESET << target_id
                << "\n";
    }
  }
  return 0;
}

int CLI::handle_set(std::span<const char *> args) {
  if (args.empty()) {
    std::cerr << "Usage: issue-tracker set <task_id> [--priority|-p <int>] "
                 "[--tags|-t <comma_separated>]\n";
    return 1;
  }
  std::string id = resolve_task_id(std::string(args[0]));
  if (id.empty())
    return 1;

  std::filesystem::path path = Config::root_dir / "tasks" / id / "TASK.md";
  if (!std::filesystem::exists(path))
    return 1;

  std::ifstream in(path);
  std::stringstream buffer;
  buffer << in.rdbuf();
  std::string text = buffer.str();
  in.close();

  int p = -1;
  std::string t = "";
  for (size_t i = 1; i < args.size(); ++i) {
    std::string arg = args[i];
    if ((arg == "-p" || arg == "--priority") && i + 1 < args.size()) {
      try {
        p = std::stoi(std::string(args[++i]));
      } catch (...) {
      }
    } else if ((arg == "-t" || arg == "--tags") && i + 1 < args.size()) {
      t = args[++i];
    }
  }

  bool changed = false;
  if (p != -1) {
    size_t end_fm = text.find("\n---");
    if (end_fm != std::string::npos) {
      std::string fm = text.substr(0, end_fm);
      size_t pos = fm.find("priority:");
      if (pos != std::string::npos) {
        size_t nl = text.find('\n', pos);
        if (nl != std::string::npos) {
          text.replace(pos, nl - pos, "priority: " + std::to_string(p));
          changed = true;
        }
      }
    }
  }
  if (!t.empty()) {
    size_t end_fm = text.find("\n---");
    if (end_fm != std::string::npos) {
      std::string fm = text.substr(0, end_fm);
      size_t pos = fm.find("tags:");
      if (pos != std::string::npos) {
        size_t nl = text.find('\n', pos);
        if (nl != std::string::npos) {
          text.replace(pos, nl - pos, "tags: [" + t + "]");
          changed = true;
        }
      }
    }
  }

  if (changed) {
    std::ofstream out(path);
    out << text;
    std::cout << colors::GREEN << "Updated metadata for " << colors::RESET << id
              << "\n";
  }
  return 0;
}

int CLI::handle_link(std::span<const char *> args) {
  if (args.size() < 2) {
    std::cerr << "Usage: <command> link <target_id> <dependency_id>\n";
    return 1;
  }
  std::string target(args[0]), dep(args[1]);

  if (!Engine::get_task_by_id(target)) {
    std::cerr << colors::RED << "Error: Target task '" << target
              << "' does not exist.\n"
              << colors::RESET;
    return 1;
  }

  if (!Engine::get_task_by_id(dep)) {
    std::cout << colors::YELLOW << "Warning: Dependency '" << dep
              << "' does not exist yet. Continue? [y/N]: " << colors::RESET;
    std::string ans;
    std::getline(std::cin, ans);
    if (ans != "y" && ans != "Y")
      return 1;
  } else {
    // Cycle guard
    std::unordered_set<std::string> visited;
    std::vector<std::string> stack = {dep};
    bool cycle = false;
    while (!stack.empty()) {
      std::string curr = stack.back();
      stack.pop_back();
      if (curr == target) {
        cycle = true;
        break;
      }
      if (visited.find(curr) == visited.end()) {
        visited.insert(curr);
        if (auto t = Engine::get_task_by_id(curr)) {
          for (const auto &d : t->depends_on)
            stack.push_back(d);
        }
      }
    }
    if (cycle) {
      std::cerr << colors::RED << "Error: Linking these would create a cycle ("
                << dep << " already depends on " << target << ").\n"
                << colors::RESET;
      return 1;
    }
  }

  modify_dependencies(target, dep, true);
  std::cout << colors::GREEN << "Linked:" << colors::RESET << " " << target
            << " now depends on " << dep << "\n";
  return 0;
}

int CLI::handle_unlink(std::span<const char *> args) {
  if (args.size() < 2) {
    std::cerr << "Usage: <command> unlink <target_id> <dependency_id>\n";
    return 1;
  }
  std::string target(args[0]), dep(args[1]);
  modify_dependencies(target, dep, false);
  std::cout << colors::GRAY << "Unlinked:" << colors::RESET << " " << target
            << " no longer depends on " << dep << "\n";
  return 0;
}

std::string CLI::create_task_file(const std::string &title,
                                  const std::string &desc, int priority,
                                  const std::string &tags,
                                  const std::string &depends) {
  std::string slug = "";
  for (char c : title) {
    if (std::isalnum(static_cast<unsigned char>(c)))
      slug += std::tolower(static_cast<unsigned char>(c));
    else if (!slug.empty() && slug.back() != '-')
      slug += '-';
  }
  if (!slug.empty() && slug.back() == '-')
    slug.pop_back();
  if (slug.empty())
    slug = "task";

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, 65535);

  std::string id;
  do {
    std::stringstream ss;
    ss << slug << "-" << std::hex << std::setfill('0') << std::setw(4)
       << dis(gen);
    id = ss.str();
  } while (std::filesystem::exists(Config::root_dir / "tasks" / id));

  std::filesystem::path task_dir = Config::root_dir / "tasks" / id;
  std::filesystem::create_directories(task_dir);

  std::string formatted_tags = tags.empty() ? "[]" : "[" + tags + "]";
  std::string formatted_deps = depends.empty() ? "[]" : "[" + depends + "]";

  std::string author = Config::get("user.name", "");
  if (author.empty()) {
    FILE *pipe = popen("git config user.name 2>/dev/null", "r");
    if (pipe) {
      char buffer[128];
      if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        author = buffer;
        if (!author.empty() && author.back() == '\n')
          author.pop_back();
      }
      pclose(pipe);
    }
    if (author.empty()) {
      const char *user_env = std::getenv("USER");
      author = user_env ? user_env : "unknown";
    }
  }

  std::ofstream file(task_dir / "TASK.md");
  file << "---\n";
  file << "status: OPEN\n";
  file << "priority: " << priority << "\n";
  file << "created_at: " << iso8601_now() << "\n";
  file << "author: " << author << "\n";
  file << "tags: " << formatted_tags << "\n";
  file << "depends_on: " << formatted_deps << "\n";
  file << "---\n";
  file << "# " << title << "\n\n";
  if (!desc.empty())
    file << desc << "\n";
  file.close();

  return id;
}

void CLI::modify_status(const std::string &id, const std::string &new_status) {
  std::filesystem::path path = Config::root_dir / "tasks" / id / "TASK.md";
  if (!std::filesystem::exists(path))
    return;

  std::ifstream in(path);
  std::stringstream buffer;
  buffer << in.rdbuf();
  std::string content = buffer.str();
  in.close();

  size_t end_fm = content.find("\n---");
  if (end_fm != std::string::npos) {
    std::string fm = content.substr(0, end_fm);
    size_t status_pos = fm.find("status:");
    if (status_pos != std::string::npos) {
      size_t newline_pos = content.find('\n', status_pos);
      if (newline_pos != std::string::npos) {
        content.replace(status_pos, newline_pos - status_pos,
                        "status: " + new_status);
        std::ofstream out(path);
        out << content;
      }
    }
  }
}

void CLI::modify_dependencies(const std::string &id, const std::string &dep_id,
                              bool add) {
  std::filesystem::path path = Config::root_dir / "tasks" / id / "TASK.md";
  if (!std::filesystem::exists(path))
    return;

  std::ifstream in(path);
  std::stringstream buffer;
  buffer << in.rdbuf();
  std::string content = buffer.str();
  in.close();

  size_t fm_end = content.find("\n---");
  if (fm_end == std::string::npos)
    fm_end = content.size();
  size_t pos = content.find("depends_on:");
  if (pos != std::string::npos && pos < fm_end) {
    size_t start_bracket = content.find('[', pos);
    size_t end_bracket = content.find(']', start_bracket);

    if (start_bracket != std::string::npos &&
        end_bracket != std::string::npos) {
      std::string array_content =
          content.substr(start_bracket + 1, end_bracket - start_bracket - 1);

      std::vector<std::string> deps;
      std::stringstream ss(array_content);
      std::string token;
      while (std::getline(ss, token, ',')) {
        size_t first = token.find_first_not_of(" \t\n\r");
        if (first != std::string::npos) {
          size_t last = token.find_last_not_of(" \t\n\r");
          deps.push_back(token.substr(first, last - first + 1));
        }
      }

      if (add) {
        if (std::find(deps.begin(), deps.end(), dep_id) == deps.end())
          deps.push_back(dep_id);
      } else {
        auto it = std::find(deps.begin(), deps.end(), dep_id);
        if (it != deps.end())
          deps.erase(it);
      }

      std::string new_array = "[";
      for (size_t i = 0; i < deps.size(); ++i) {
        new_array += deps[i];
        if (i < deps.size() - 1)
          new_array += ", ";
      }
      new_array += "]";

      content.replace(start_bracket, end_bracket - start_bracket + 1,
                      new_array);
      std::ofstream out(path);
      out << content;
    }
  }
}

void CLI::recursive_rm(const std::string &id,
                       std::unordered_set<std::string> &deleted) {
  if (deleted.find(id) != deleted.end())
    return;
  deleted.insert(id);

  std::vector<Task> all_tasks = Engine::get_all_tasks();
  for (const Task &t : all_tasks) {
    if (std::find(t.depends_on.begin(), t.depends_on.end(), id) !=
        t.depends_on.end()) {
      recursive_rm(t.id, deleted);
    }
  }

  std::filesystem::path dir = Config::root_dir / "tasks" / id;
  if (std::filesystem::exists(dir)) {
    std::filesystem::remove_all(dir);
    std::cout << colors::RED << "Deleted: " << colors::RESET << id << "\n";
  }
}

int CLI::handle_start(std::span<const char *> args) {
  bool do_branch = false;
  std::vector<std::string> clean_args;
  
  for (size_t i = 0; i < args.size(); ++i) {
    std::string s(args[i]);
    if (s == "-b" || s == "--branch") {
      do_branch = true;
    } else {
      clean_args.push_back(s);
    }
  }

  std::vector<const char*> span_args;
  for (const auto& a : clean_args) span_args.push_back(a.c_str());

  std::string id = get_target_task_id(std::span<const char*>(span_args.data(), span_args.size()));
  if (id.empty()) {
    std::cerr << "Usage: <command> start [<id>] [-b|--branch]\n";
    return 1;
  }

  std::optional<Task> task = Engine::get_task_by_id(id);
  if (!task.has_value()) {
    std::cerr << "Task not found.\n";
    return 1;
  }

  Config::set("active_task", id);

  if (do_branch) {
    std::string branch_name = "task/" + id;
    std::string cmd = "git checkout -b " + escape_shell(branch_name) + " > /dev/null 2>&1 || git checkout " + escape_shell(branch_name) + " > /dev/null 2>&1";
    std::system(cmd.c_str());
    std::cout << colors::CYAN << "Switched to branch " << branch_name << ".\n" << colors::RESET;
  } else {
    std::cout << colors::CYAN << "Context set to task " << id << ".\n" << colors::RESET;
  }
  std::cout << "Happy coding!\n";
  return 0;
}

int CLI::handle_finish(std::span<const char *> args) {
  std::string inline_msg = "";
  bool open_editor = false;
  bool stage_all = Config::get("git.autostage_code", "false") == "true";
  bool stage_tasks = Config::get("git.autostage_tasks", "false") == "true";
  bool do_pr = false;
  
  std::vector<std::string> clean_args;
  for (size_t i = 0; i < args.size(); ++i) {
    std::string arg = args[i];
    if (arg == "-m" || arg == "--message") {
      if (i + 1 < args.size()) inline_msg = args[++i];
    } else if (arg == "-e" || arg == "--edit") {
      open_editor = true;
    } else if (arg == "-a" || arg == "--stage-all") {
      stage_all = true;
    } else if (arg == "--stage-tasks") {
      stage_tasks = true;
    } else if (arg == "--pr") {
      do_pr = true;
    } else {
      clean_args.push_back(arg);
    }
  }
  
  std::vector<const char*> span_args;
  for (const auto& a : clean_args) span_args.push_back(a.c_str());

  std::string id = get_target_task_id(std::span<const char*>(span_args.data(), span_args.size()));
  if (id.empty()) {
    std::cerr << "Usage: <command> finish [<id>] [-m <message>] [-e] [-a] [--stage-tasks] [--pr]\n";
    return 1;
  }

  std::optional<Task> task = Engine::get_task_by_id(id);
  if (!task.has_value()) {
    std::cerr << "Task not found.\n";
    return 1;
  }

  modify_status(id, "CLOSED");

  std::string msg = "[CLOSED] #" + id + ": ";
  if (!inline_msg.empty()) {
      msg += inline_msg;
  } else {
      msg += task.value().title;
  }

  std::string template_path = "/tmp/tracker_commit_template.txt";
  std::ofstream out(template_path);
  out << msg << "\n";
  if (open_editor && inline_msg.empty()) out << "\n- \n";
  out.close();

  std::string add_tasks = escape_shell((Config::root_dir / "tasks").string());
  std::string add_config = escape_shell((Config::root_dir / ".trackerconfig").string());
  
  if (stage_all) {
      std::system(("git add -A > /dev/null 2>&1"));
      std::cout << colors::GRAY << "Staged all code changes.\n" << colors::RESET;
  } else if (stage_tasks) {
      std::system(("git add " + add_tasks + " " + add_config + " > /dev/null 2>&1").c_str());
      std::cout << colors::GRAY << "Staged task metadata.\n" << colors::RESET;
  }

  std::string cmd = "git commit -F " + template_path;
  if (open_editor) cmd = "git commit -e -F " + template_path;
  
  int status = std::system(cmd.c_str());
  std::filesystem::remove(template_path);
  
  if (status != 0) {
      std::cerr << colors::RED << "Commit aborted or nothing to commit. Reverting task to OPEN.\n" << colors::RESET;
      modify_status(id, "OPEN");
      if (stage_tasks || stage_all) {
          std::system(("git reset HEAD " + add_tasks + " " + add_config + " > /dev/null 2>&1").c_str());
      }
      return 1;
  }

  std::cout << colors::GREEN << "Closed task: " << colors::RESET << id << "\n";

  if (do_pr) {
      // For PR, we need to ensure we are pushing something.
      // Usually they'd use --pr on a branch, or it creates one.
      // If we want to strictly mimic "finish --pr" pushing the current branch:
      std::cout << colors::CYAN << "Pushing to origin...\n" << colors::RESET;
      
      FILE *pipe = popen("git branch --show-current 2>/dev/null", "r");
      char buffer[256];
      std::string current_branch;
      if (pipe && fgets(buffer, sizeof(buffer), pipe) != nullptr) {
          current_branch = buffer;
          if (!current_branch.empty() && current_branch.back() == '\n')
              current_branch.pop_back();
      }
      if (pipe) pclose(pipe);
      
      if (!current_branch.empty()) {
          std::string push_cmd = "git push -u origin " + escape_shell(current_branch);
          std::system(push_cmd.c_str());
      }
  }

  // Clear active task if it was this one
  if (Config::get("active_task", "") == id) {
      Config::set("active_task", "");
  }

  return 0;
}

int CLI::handle_submit(std::span<const char *> args) {
  if (std::system("git remote get-url origin > /dev/null 2>&1") != 0) {
    std::cerr << colors::RED << "Error: " << colors::RESET << "No 'origin' remote configured.\n";
    std::cerr << "To push this branch, add a remote repository first:\n";
    std::cerr << "  git remote add origin <url>\n";
    return 1;
  }

  std::optional<std::string> id_opt = get_active_task();
  
  std::string id = "";
  if (args.size() > 0) {
      id = resolve_task_id(args[0]);
      if (id.empty()) return 1;
  } else if (id_opt.has_value()) {
      id = id_opt.value();
  } else {
      std::cerr << colors::RED << "Error: " << colors::RESET << "You are not on a task branch.\n";
      std::cerr << "Run 'issue-tracker submit <id>' to specify a task, or switch to its branch.\n";
      return 1;
  }

  std::string branch_name = "task/" + id;
  std::cout << colors::CYAN << "Pushing " << branch_name << " to origin...\n" << colors::RESET;

  std::string cmd = "git push -u origin " + escape_shell(branch_name);
  int status = std::system(cmd.c_str());

  if (status == 0) {
      std::cout << colors::GREEN << "\nSuccess! " << colors::RESET << "Branch pushed to remote.\n";
      std::cout << colors::GRAY << "Check the output above for your Pull Request link.\n" << colors::RESET;
  } else {
      std::cerr << colors::RED << "\nFailed to push branch.\n" << colors::RESET;
      return 1;
  }

  return 0;
}

} // namespace tracker

