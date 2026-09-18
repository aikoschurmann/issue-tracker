#include "tracker/cli.hpp"
#include <vector>

int main(int argc, const char **argv) {
  // Skip the executable name (argv[0]) and pass the rest to the CLI router
  std::vector<const char *> args(argv + 1, argv + argc);
  return tracker::CLI::run(args);
}
