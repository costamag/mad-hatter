// experiments/opto/resynthesis/parser.hpp
#pragma once
#include <cstdio>
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <stdexcept>

namespace rinox
{

namespace experiments
{

template<class Params>
struct resynthesis_t
{
  Params ps;
  std::string input;
};

template<class Params>
inline resynthesis_t<Params> load_resynthesis_params( rapidjson::Document const& doc, lorina::diagnostic_engine * diag )
{
  resynthesis_t<Params> res;
  Params & ps = res.ps;
  
  if ( !doc.HasMember( "resynthesis" ) || !doc["resynthesis"].IsObject() )
    return res;

  const auto& rp = doc["resynthesis"];

  if ( rp.HasMember( "profiler_ps" ) && rp["profiler_ps"].IsObject() )
  {
    const auto& p = rp["profiler_ps"];
    if ( p.HasMember( "max_num_roots" ) && p["max_num_roots"].IsUint() )
      ps.profiler_ps.max_num_roots = p["max_num_roots"].GetUint();

    if ( p.HasMember( "input_arrivals" ) && p["input_arrivals"].IsDouble() )
      ps.profiler_ps.input_arrivals = { p["input_arrivals"].GetDouble() };

    if ( p.HasMember( "output_required" ) )
    {
      if ( p["output_required"].IsString() && std::string( p["output_required"].GetString() ) == "INF" )
      {
        ps.profiler_ps.output_required = { -1 };
      }
      else if ( p["output_required"].IsDouble() )
      {
        ps.profiler_ps.output_required = { p["output_required"].GetDouble() };
      }
    }

    if ( p.HasMember( "eps" ) && p["eps"].IsNumber() )
      ps.profiler_ps.eps = p["eps"].GetDouble();
  }

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

  return res;
}

} // namespace experiments

} // namespace rinox
