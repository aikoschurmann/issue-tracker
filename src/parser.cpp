#include "tracker/parser.hpp"
#include <algorithm>
#include <cctype>
#include <charconv>

namespace tracker {

// =========================================================================
// LEVEL 1: High-Level Orchestration
// =========================================================================

std::optional<Task> Parser::parse_task(std::string_view content) {
  Task task;
  task.status = Status::Open;
  task.priority = 0;

  std::string_view frontmatter = extract_frontmatter(content);
  parse_frontmatter_block(task, frontmatter);

  task.title = extract_title(content);
  task.body = extract_body(content);

  return task;
}

// =========================================================================
// LEVEL 2: Block & Section Extraction
// =========================================================================

std::string_view Parser::extract_frontmatter(std::string_view &content) {
  content = trim(content);
  if (!content.starts_with("---"))
    return "";

  get_line(content); // consume the first '---'

  size_t end_pos = content.find("\n---");
  if (end_pos == std::string_view::npos)
    return "";

  std::string_view frontmatter = content.substr(0, end_pos);

  // Advance the original content past the closing '---'
  content = content.substr(end_pos + 1);
  get_line(content);

  return frontmatter;
}

void Parser::parse_frontmatter_block(Task &task, std::string_view frontmatter) {
  while (!frontmatter.empty()) {
    std::string_view line = get_line(frontmatter);

    std::pair<std::string_view, std::string_view> kv = split_key_value(line);
    std::string_view key = kv.first;
    std::string_view val = kv.second;

    if (key.empty())
      continue;

    if (key == "status")
      task.status = parse_status(val);
    if (key == "priority")
      task.priority = parse_priority(val);
    if (key == "created_at")
      task.created_at = std::string(val);
    if (key == "author")
      task.author = std::string(val);
    if (key == "tags")
      task.tags = parse_list(val);
    if (key == "depends_on")
      task.depends_on = parse_list(val);
  }
}

std::string Parser::extract_title(std::string_view &markdown) {
  markdown = trim(markdown);
  if (!markdown.starts_with("#"))
    return "Untitled Task";

  std::string_view title_line = get_line(markdown);
  title_line.remove_prefix(1); // remove '#'
  return std::string(trim(title_line));
}

std::string Parser::extract_body(std::string_view markdown) {
  return std::string(trim(markdown));
}

// =========================================================================
// LEVEL 3: Domain Data Parsing
// =========================================================================

Status Parser::parse_status(std::string_view val) {
  if (iequals(val, "closed")) {
    return Status::Closed;
  }
  return Status::Open;
}

int Parser::parse_priority(std::string_view val) {
  int priority = 0;
  std::from_chars(val.data(), val.data() + val.size(), priority);
  return priority;
}

std::vector<std::string> Parser::parse_list(std::string_view val) {
  std::vector<std::string> list;

  if (val.starts_with("["))
    val.remove_prefix(1);
  if (val.ends_with("]"))
    val.remove_suffix(1);

  size_t start = 0;
  while (start < val.size()) {
    size_t comma = val.find(',', start);
    if (comma == std::string_view::npos)
      comma = val.size();

    std::string_view item = trim(val.substr(start, comma - start));
    if (item.starts_with("'") || item.starts_with("\""))
      item.remove_prefix(1);
    if (item.ends_with("'") || item.ends_with("\""))
      item.remove_suffix(1);
    if (!item.empty()) {
      list.push_back(std::string(item));
    }
    start = comma + 1;
  }
  return list;
}

// =========================================================================
// LEVEL 4: Low-Level String Operations
// =========================================================================

std::string_view Parser::get_line(std::string_view &content) {
  size_t pos = content.find('\n');
  if (pos == std::string_view::npos) {
    std::string_view line = content;
    content = "";
    return line;
  }
  std::string_view line = content.substr(0, pos);
  content = content.substr(pos + 1);
  return line;
}

std::string_view Parser::trim(std::string_view sv) {
  sv.remove_prefix(std::min(sv.find_first_not_of(" \t\r\n"), sv.size()));
  sv.remove_suffix(sv.size() -
                   std::min(sv.find_last_not_of(" \t\r\n") + 1, sv.size()));
  return sv;
}

bool Parser::iequals(std::string_view a, std::string_view b) {
  return std::equal(a.begin(), a.end(), b.begin(), b.end(), [](char a, char b) {
    return std::tolower(static_cast<unsigned char>(a)) ==
           std::tolower(static_cast<unsigned char>(b));
  });
}

std::pair<std::string_view, std::string_view>
Parser::split_key_value(std::string_view line) {
  line = trim(line);
  size_t colon_pos = line.find(':');
  if (colon_pos == std::string_view::npos)
    return {"", ""};

  std::string_view key = trim(line.substr(0, colon_pos));
  std::string_view val = trim(line.substr(colon_pos + 1));
  return {key, val};
}

} // namespace tracker
