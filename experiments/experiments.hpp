#pragma once
// bench_paths.cpp
#include <filesystem>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#ifndef RINOX_BENCHMARK_DIR
#define RINOX_BENCHMARK_DIR "../benchmarks"
#endif

namespace rinox
{

namespace experiments
{

namespace fs = std::filesystem;

std::string get_experiments_root()
{
  if ( const char* env = std::getenv( "RINOX_EXPERIMENTS_DIR" ) )
  {
    return env;
  }
  return RINOX_EXPERIMENTS_DIR;
}

inline std::vector<std::string> collect_suite_files( const std::string& suite,
                                                     const std::string& ext /* e.g. ".aig" */ )
{
  std::vector<std::string> out;
  fs::path base = fs::path( RINOX_BENCHMARK_DIR ) / suite;
  if ( !fs::exists( base ) )
    return out;
  for ( const auto& e : fs::directory_iterator( base ) )
  {
    if ( e.is_regular_file() && e.path().extension() == ext )
    {
      out.push_back( e.path().string() );
    }
  }
  return out;
}

} // namespace experiments

} // namespace rinox