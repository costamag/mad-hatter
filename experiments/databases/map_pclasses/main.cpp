#include "parser.hpp"
#include <mockturtle/io/genlib_reader.hpp>
#include <rinox/databases/mapped_database.hpp>
#include <rinox/databases/database_generator.hpp>

int main()
{
  using rinox::experiments::BenchSpec;
  using rinox::experiments::TechSpec;
  rinox::databases::database_gen_params ps;
  BenchSpec spec;
  TechSpec tech;
  std::string config_path = std::string( RINOX_EXPERIMENTS_DIR ) + "/databases/map_pclasses/config.json";
  try
  {
    rinox::experiments::load_config( config_path, ps, spec, tech );
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

  rinox::databases::database_generator gen( gates );
  ps.output_file = std::string( RINOX_SOURCE_DIR ) + "/databases/" + tech.name + "/" + tech.name + "_" + ps.metric + "_" + std::to_string(ps.num_vars);
  gen.run( ps );

  return 0;
}