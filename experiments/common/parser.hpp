#pragma once
#include <filesystem>
#include <set>
#include <string>
#include <vector>

namespace rinox::experiments
{

namespace fs = std::filesystem;

#ifndef RINOX_BENCHMARK_DIR
#define RINOX_BENCHMARK_DIR "benchmarks"
#endif

// -------- Data structures --------
struct BenchSpec
{
  std::string type = "aiger";       // "aig"/"aiger" | "verilog" | "blif" | "aag" ...
  std::vector<std::string> suites;  // e.g., ["iscas", "epfl"]
  std::vector<std::string> names;   // optional: include-only names (base names)
  std::vector<std::string> exclude; // optional: names to exclude
  std::string root;                 // optional override; default = RINOX_BENCHMARK_DIR
};

// -------- Non-template helpers (defined in .cpp) --------
bool read_text_list_file( const fs::path& path, std::vector<std::string>& out_lines );
fs::path default_suite_list_file( const fs::path& suite_dir, const std::string& suite_name );
void normalize_unique( std::vector<std::string>& v );
std::string normalize_ext( std::string type ); // returns ".aig", ".v", ".blif", ...

// Collect file paths from a suite directory, given ext (".aig"), and optional filters.
void collect_from_suite( const fs::path& suite_dir,
                         const std::string& suite_name,
                         const std::string& ext,
                         const std::set<std::string>& names_filter, // empty = all
                         const std::set<std::string>& exclude_set,
                         std::vector<std::string>& out_files );

// -------- Loader for just the "benchmarks" block --------
// Parses JSON at json_path, fills BenchSpec + resolved file list.
// Note: experiment-specific parameter parsing happens elsewhere.
void load_common_config( const std::string& json_path,
                         BenchSpec& spec_out,
                         std::vector<std::string>& files_out );

} // namespace rinox::experiments
