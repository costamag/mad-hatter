#include "json.hpp"
#include "../../../../include/rinox/io/json/json.hpp"
#include "../../../../include/rinox/network/network.hpp"
#include "../../context.hpp"
#include <fstream>
#include <iostream>
#include <lorina/verilog.hpp>

static void cmd_read_json( CLIContext& ctx, const std::vector<std::string>& args )
{
  if ( args.size() < 2 )
  {
    std::cerr << "Usage: read_json <filename>\n";
    return;
  }
  if ( ctx.gates.empty() )
  {
    std::cerr << "Error: load a library first with read_genlib.\n";
    return;
  }
  std::ifstream in_ntk( args[1] );
  if ( !in_ntk )
  {
    std::cerr << "Cannot open " << args[1] << "\n";
    return;
  }
  ctx.ntk.emplace( ctx.gates );

  lorina::text_diagnostics consumer;
  lorina::diagnostic_engine diag( &consumer );

  auto result_ntk = rinox::io::json::read_json(
      in_ntk, rinox::io::reader( *ctx.ntk ), &diag );
  if ( result_ntk == lorina::return_code::success )
    std::cout << "Design loaded.\n";
  else
    std::cerr << "Failed to read verilog.\n";
}

std::map<std::string, CommandHandler> register_json_commands()
{
  return {
      { "read_json", cmd_read_json } };
}
