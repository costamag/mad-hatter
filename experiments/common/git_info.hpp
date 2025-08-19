#pragma once
#include <string>
namespace rinox::experiments
{
struct GitInfo
{
  std::string sha;
  bool dirty = false;
  std::string branch;
};
GitInfo get_git_info(); // call `git rev-parse` via popen or skip if unavailable
} // namespace rinox::experiments
