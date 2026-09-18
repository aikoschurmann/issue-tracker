#pragma once
#include <filesystem>
#include <string>
#include <unordered_map>

namespace tracker {

enum class RootState { Found, GitFallback, NotFound };

class Config {
public:
  inline static std::filesystem::path root_dir = ".";

  // Loads config from ~/.trackerconfig and ./.trackerconfig
  static RootState find_root();
  static void load();

  // Gets a config value, returns default_val if not found
  static std::string get(const std::string &key,
                         const std::string &default_val = "");
  static std::string get_global(const std::string &key,
                                const std::string &default_val = "");

  // Sets a config value and writes it to ./.trackerconfig
  static void set(const std::string &key, const std::string &value);

private:
  inline static std::unordered_map<std::string, std::string> global_settings;
  inline static std::unordered_map<std::string, std::string> local_settings;
  static void
  load_file(const std::filesystem::path &path,
            std::unordered_map<std::string, std::string> &target_map);
  static void
  save_file(const std::filesystem::path &path,
            const std::unordered_map<std::string, std::string> &settings);
};

} // namespace tracker
