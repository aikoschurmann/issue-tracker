#pragma once
#include "tracker/task.hpp"
#include <optional>
#include <string_view>

namespace tracker {

class Parser {
public:
  // Parse a TASK.md file content into a Task object.
  // Returns std::nullopt if the formatting is invalid.
  static std::optional<Task> parse_task(std::string_view content);

private:
  // High-level extraction
  static std::string_view extract_frontmatter(std::string_view &content);
  static void parse_frontmatter_block(Task &task, std::string_view frontmatter);

  // Domain-level parsing
  static Status parse_status(std::string_view val);
  static int parse_priority(std::string_view val);
  static std::vector<std::string> parse_list(std::string_view val);

  static std::string extract_title(std::string_view &markdown);
  static std::string extract_body(std::string_view markdown);

  // Low-level string manipulation
  static std::string_view get_line(std::string_view &content);
  static std::string_view trim(std::string_view sv);
  static bool iequals(std::string_view a, std::string_view b);
  static std::pair<std::string_view, std::string_view>
  split_key_value(std::string_view line);
};

} // namespace tracker
