#pragma once
#include <string>
#include <vector>
#include <map>

namespace rinox::experiments
{

  struct RunMeta {
    std::string experiment;
    std::string config_path;
    std::string git_sha;
    bool        git_dirty = false;
    std::string hostname;
    std::string compiler;
    std::string timestamp_utc;
    unsigned    rng_seed = 0;
    std::map<std::string,std::string> build_defs;
  };

  void write_results_json(const std::string& out_path,
                          const RunMeta& meta,
                          const std::string& params_json_snippet,
                          const std::string& results_json_snippet);
}
