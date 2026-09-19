#pragma once
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include "tracker/config.hpp"

namespace tracker {

inline std::string iso8601_now() {
  auto now = std::chrono::system_clock::now();
  std::time_t now_c = std::chrono::system_clock::to_time_t(now);
  std::stringstream ss;
  ss << std::put_time(std::gmtime(&now_c), "%Y-%m-%dT%H:%M:%SZ");
  return ss.str();
}

inline std::optional<std::string> get_active_task() {
  std::string active = Config::get("active_task", "");
  if (!active.empty()) {
    return active;
  }
  
  // Fallback to git branch for backwards compatibility (temporary)
  FILE *pipe = popen("git branch --show-current 2>/dev/null", "r");
  if (!pipe)
    return std::nullopt;
  char buffer[256];
  std::string branch;
  if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    branch = buffer;
    if (!branch.empty() && branch.back() == '\n')
      branch.pop_back();
  }
  pclose(pipe);

  std::string prefix = "task/";
  if (branch.find(prefix) == 0) {
    return branch.substr(prefix.length());
  }
  return std::nullopt;
}

inline std::string pad_truncate(const std::string &str, size_t width) {
  if (str.length() > width) {
    return str.substr(0, width - 3) + "...";
  }
  return str + std::string(width - str.length(), ' ');
}

inline std::string escape_shell(const std::string &str) {
  std::string escaped = "'";
  for (char c : str) {
    if (c == '\'')
      escaped += "'\\''";
    else
      escaped += c;
  }
  escaped += "'";
  return escaped;
}
} // namespace tracker
