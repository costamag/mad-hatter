// experiments/opto/resynthesis/parser.hpp
#pragma once
#include "../common/parser.hpp" // load_common_config(...)
#include <cstdio>
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <stdexcept>

namespace rinox
{

namespace experiments
{

template<class Params>
inline void load_resynthesis_params( const std::string& json_path, Params& ps )
{
  // Parse JSON (second pass focused on "resynthesis-params")
  FILE* fp = std::fopen( json_path.c_str(), "rb" );
  if ( !fp )
    throw std::runtime_error( "Cannot open config: " + json_path );

  char buf[1 << 16];
  rapidjson::FileReadStream is( fp, buf, sizeof( buf ) );
  rapidjson::Document doc;
  doc.ParseStream( is );
  std::fclose( fp );

  if ( !doc.IsObject() )
    throw std::runtime_error( "Config is not a JSON object: " + json_path );
  if ( !doc.HasMember( "resynthesis-params" ) || !doc["resynthesis-params"].IsObject() )
    return;

  const auto& rp = doc["resynthesis-params"];

  // profiler_ps
  if ( rp.HasMember( "profiler_ps" ) && rp["profiler_ps"].IsObject() )
  {
    const auto& p = rp["profiler_ps"];
    if ( p.HasMember( "max_num_roots" ) && p["max_num_roots"].IsUint() )
      ps.profiler_ps.max_num_roots = p["max_num_roots"].GetUint();

    // Adjust these to your real types (scalar vs vector/optional).
    if ( p.HasMember( "input_arrivals" ) && p["input_arrivals"].IsUint() )
      ps.profiler_ps.input_arrivals = { p["input_arrivals"].GetUint() };

    if ( p.HasMember( "output_required" ) )
    {
      if ( p["output_required"].IsString() && std::string( p["output_required"].GetString() ) == "INF" )
      {
        // choose a sentinel appropriate for your type
        ps.profiler_ps.output_required = { -1 };
      }
      else if ( p["output_required"].IsInt() )
      {
        ps.profiler_ps.output_required = { p["output_required"].GetInt() };
      }
    }

    if ( p.HasMember( "eps" ) && p["eps"].IsNumber() )
      ps.profiler_ps.eps = p["eps"].GetDouble();
  }

  // window_manager_ps
  if ( rp.HasMember( "window_manager_ps" ) && rp["window_manager_ps"].IsObject() )
  {
    const auto& wm = rp["window_manager_ps"];
    if ( wm.HasMember( "preserve_depth" ) && wm["preserve_depth"].IsBool() )
      ps.window_manager_ps.preserve_depth = wm["preserve_depth"].GetBool();
    if ( wm.HasMember( "odc_levels" ) && wm["odc_levels"].IsInt() )
      ps.window_manager_ps.odc_levels = wm["odc_levels"].GetInt();
    if ( wm.HasMember( "skip_fanout_limit_for_divisors" ) && wm["skip_fanout_limit_for_divisors"].IsUint() )
      ps.window_manager_ps.skip_fanout_limit_for_divisors = wm["skip_fanout_limit_for_divisors"].GetUint();
    if ( wm.HasMember( "max_num_divisors" ) && wm["max_num_divisors"].IsUint() )
      ps.window_manager_ps.max_num_divisors = wm["max_num_divisors"].GetUint();
  }

  // top-level flags in resynthesis-params
  if ( rp.HasMember( "use_dont_cares" ) && rp["use_dont_cares"].IsBool() )
    ps.use_dont_cares = rp["use_dont_cares"].GetBool();
  if ( rp.HasMember( "try_rewire" ) && rp["try_rewire"].IsBool() )
    ps.try_rewire = rp["try_rewire"].GetBool();
  if ( rp.HasMember( "try_struct" ) && rp["try_struct"].IsBool() )
    ps.try_struct = rp["try_struct"].GetBool();
  if ( rp.HasMember( "try_window" ) && rp["try_window"].IsBool() )
    ps.try_window = rp["try_window"].GetBool();
  if ( rp.HasMember( "try_simula" ) && rp["try_simula"].IsBool() )
    ps.try_simula = rp["try_simula"].GetBool();
  if ( rp.HasMember( "dynamic_database" ) && rp["dynamic_database"].IsBool() )
    ps.dynamic_database = rp["dynamic_database"].GetBool();
  if ( rp.HasMember( "fanout_limit" ) && rp["fanout_limit"].IsUint() )
    ps.fanout_limit = rp["fanout_limit"].GetUint();
}

template<class Params>
inline void load_config( const std::string& path,
                         Params& ps,
                         rinox::experiments::BenchSpec& spec,
                         rinox::experiments::TechSpec& tech,
                         std::vector<std::string>& files )
{
  // 1) benchmarks (common)
  rinox::experiments::load_common_config( path, spec, tech, files );

  // 2) resynthesis-specific params
  load_resynthesis_params( path, ps );
}

} // namespace experiments
} // namespace rinox
