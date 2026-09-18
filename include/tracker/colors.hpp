#pragma once
#include <string_view>

namespace tracker {
namespace colors {

constexpr std::string_view RED = "\x1b[31m";
constexpr std::string_view GREEN = "\x1b[32m";
constexpr std::string_view YELLOW = "\x1b[33m";
constexpr std::string_view GRAY = "\x1b[90m";
constexpr std::string_view CYAN = "\x1b[36m";

constexpr std::string_view BOLD = "\x1b[1m";
constexpr std::string_view DIM = "\x1b[2m";
constexpr std::string_view RESET = "\x1b[0m";

} // namespace colors
} // namespace tracker
