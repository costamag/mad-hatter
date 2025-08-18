#include "../common/env.hpp"
#include "parser.hpp"
#include <iostream>
#include <rinox/opto/algorithms/resynthesize.hpp>
#include <string>
#include <vector>

#include <fmt/format.h>
#include <lorina/aiger.hpp>
#include <lorina/genlib.hpp>
#include <mockturtle/algorithms/aig_balancing.hpp>
#include <mockturtle/algorithms/emap.hpp>
#include <mockturtle/io/aiger_reader.hpp>
#include <mockturtle/io/genlib_reader.hpp>
#include <mockturtle/io/write_verilog.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/block.hpp>
#include <mockturtle/utils/name_utils.hpp>
#include <mockturtle/utils/tech_library.hpp>
#include <mockturtle/views/cell_view.hpp>
#include <mockturtle/views/depth_view.hpp>
#include <mockturtle/views/names_view.hpp>

using rinox::experiments::BenchSpec;
using rinox::experiments::TechSpec;

template<uint32_t NL = RINOX_MAX_NUM_LEAVES>
using DefaultParams = rinox::opto::algorithms::default_resynthesis_params<NL>;

int main()
{
  std::string path_to_out = std::string( RINOX_SOURCE_DIR ) + "/experiments/resynthesis/";

  rinox::experiments::experiment<std::string, uint32_t, double, uint32_t, double, uint32_t, float, bool> exp(
      path_to_out, "resynthesis", "benchmark", "size", "area_after", "depth", "delay_after", "multioutput", "runtime", "cec" );

  DefaultParams<> ps;
  BenchSpec spec;
  TechSpec tech;
  std::vector<std::string> files;
  std::string config_path = std::string( RINOX_EXPERIMENTS_DIR ) + "/resynthesis/config.json";
  try
  {
    rinox::experiments::load_config( config_path, ps, spec, tech, files );
  }
  catch ( const std::exception& e )
  {
    std::cerr << "config error: " << e.what() << "\n";
    return 1;
  }

  /* library to map to technology */
  fmt::print( "[i] processing technology library\n" );
  std::string library = tech.name;
  std::vector<mockturtle::gate> gates;

  std::string genlib_path = std::string( RINOX_SOURCE_DIR ) + "/techlib/" + tech.type + "/" + tech.name + "." + tech.type;

  std::ifstream in( genlib_path );

  if ( lorina::read_genlib( in, mockturtle::genlib_reader( gates ) ) != lorina::return_code::success )
  {
    return 1;
  }

  mockturtle::tech_library_params tps;
  tps.ignore_symmetries = false; // set to true to drastically speed-up mapping with minor delay increase
  tps.verbose = true;
  mockturtle::tech_library<9> tech_lib( gates, tps );

  int cnt = 0;
  for ( auto& f : files )
  {

    mockturtle::names_view<mockturtle::aig_network> aig;
    if ( lorina::read_aiger( f, mockturtle::aiger_reader( aig ) ) != lorina::return_code::success )
    {
      continue;
    }
    /* remove structural redundancies */
    mockturtle::aig_balancing_params bps;
    bps.minimize_levels = false;
    bps.fast_mode = true;
    mockturtle::aig_balance( aig, bps );

    const uint32_t size_before = aig.num_gates();
    const uint32_t depth_before = mockturtle::depth_view( aig ).depth();

    mockturtle::emap_params ps;
    ps.matching_mode = mockturtle::emap_params::hybrid;
    ps.area_oriented_mapping = false;
    ps.map_multioutput = true;
    ps.relax_required = 0;
    mockturtle::emap_stats st;
    mockturtle::cell_view<mockturtle::block_network> res = mockturtle::emap<9>( aig, tech_lib, ps, &st );

    mockturtle::names_view res_names{ res };
    mockturtle::restore_network_name( aig, res_names );
    mockturtle::restore_pio_names_by_order( aig, res_names );
    const auto cec = rinox::experiments::abc_cec_mapped_cell( res_names, f, genlib_path );

    exp( spec.names[cnt], size_before, res.compute_area(), depth_before, res.compute_worst_delay(), st.multioutput_gates, mockturtle::to_seconds( st.time_total ), cec );
    cnt++;
  }
  exp.save();
  exp.table();
  return 0;
}