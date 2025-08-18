#include "parser.hpp"
#include <iostream>
#include <rinox/opto/algorithms/resynthesize.hpp>

using rinox::experiments::BenchSpec;

template<uint32_t NL = RINOX_MAX_NUM_LEAVES>
using DefaultParams = rinox::opto::algorithms::default_resynthesis_params<NL>;

int main()
{
  DefaultParams<> ps;
  BenchSpec spec;
  std::vector<std::string> files;

  try
  {
    rinox::experiments::load_config( "experiments/resynthesis/config.json", ps, spec, files );
  }
  catch ( const std::exception& e )
  {
    std::cerr << "config error: " << e.what() << "\n";
    return 1;
  }

  std::cout << "type=" << spec.type << " suites=" << spec.suites.size()
            << " files=" << files.size() << "\n";
  for ( auto& f : files )
    std::cout << "  " << f << "\n";
  return 0;
}