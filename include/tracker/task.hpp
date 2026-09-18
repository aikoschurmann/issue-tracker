#pragma once
#include <string>
#include <vector>

namespace tracker {

enum class Status { Open, Closed };

struct Task {
  std::string id;
  std::string title;
  Status status;
  int priority;
  std::string created_at;
  std::string author;
  std::vector<std::string> tags;
  std::vector<std::string> depends_on;
  std::string body;
};

} // namespace tracker
