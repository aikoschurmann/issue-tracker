#include "tracker/config.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>

namespace tracker {

RootState Config::find_root() {
  std::filesystem::path curr = std::filesystem::current_path();
  std::filesystem::path git_fallback = "";
  while (true) {
    if (std::filesystem::exists(curr / ".trackerconfig")) {
      root_dir = curr;
      return RootState::Found;
    }
    if (git_fallback.empty() && std::filesystem::exists(curr / ".git")) {
      git_fallback = curr;
    }
    if (curr == curr.parent_path())
      break;
    curr = curr.parent_path();
  }
  if (!git_fallback.empty()) {
    root_dir = git_fallback;
    return RootState::GitFallback;
  }
  root_dir = std::filesystem::current_path();
  return RootState::NotFound;
}

void Config::load() {
  const char *home = std::getenv("HOME");
  if (home) {
    std::filesystem::path global_path =
        std::filesystem::path(home) / ".trackerconfig";
    load_file(global_path, global_settings);
  }
  std::filesystem::path local_path = root_dir / ".trackerconfig";
  load_file(local_path, local_settings);
}

std::string Config::get(const std::string &key,
                        const std::string &default_val) {
  if (local_settings.find(key) != local_settings.end()) {
    return local_settings[key];
  }
  if (global_settings.find(key) != global_settings.end()) {
    return global_settings[key];
  }
  return default_val;
}

std::string Config::get_global(const std::string &key,
                               const std::string &fallback) {
  if (global_settings.find(key) != global_settings.end()) {
    return global_settings[key];
  }
  return fallback;
}
void Config::set(const std::string &key, const std::string &value) {
  local_settings[key] = value;
  std::filesystem::path local_path = root_dir / ".trackerconfig";
  save_file(local_path, local_settings);
}

void Config::load_file(
    const std::filesystem::path &path,
    std::unordered_map<std::string, std::string> &target_map) {
  if (!std::filesystem::exists(path))
    return;

  std::ifstream file(path);
  std::string line;
  while (std::getline(file, line)) {
    // Very simple parser for key=value
    size_t eq = line.find('=');
    if (eq != std::string::npos) {
      std::string key = line.substr(0, eq);
      std::string val = line.substr(eq + 1);

      // Trim
      auto trim = [](std::string &s) {
        s.erase(0, s.find_first_not_of(" \t\r"));
        s.erase(s.find_last_not_of(" \t\r") + 1);
      };
      trim(key);
      trim(val);

      if (!key.empty()) {
        target_map[key] = val;
      }
    }
  }
}

void Config::save_file(
    const std::filesystem::path &path,
    const std::unordered_map<std::string, std::string> &settings) {
  std::ofstream file(path);
  for (const auto &[k, v] : settings) {
    file << k << "=" << v << "\n";
  }
}

} // namespace tracker
