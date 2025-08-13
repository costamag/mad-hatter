#include "verilog.hpp"
#include "../../../../include/rinox/io/verilog/verilog.hpp"
#include "../../../../include/rinox/network/network.hpp"
#include "../../context.hpp"
#include <fstream>
#include <iostream>
#include <lorina/verilog.hpp>

static void cmd_read_verilog( CLIContext& ctx, const std::vector<std::string>& args )
{
  if ( args.size() < 2 )
  {
    std::cerr << "Usage: read_verilog <filename>\n";
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

  auto result_ntk = rinox::io::verilog::read_verilog(
      in_ntk, rinox::io::reader( *ctx.ntk ), &diag );
  if ( result_ntk == lorina::return_code::success )
    std::cout << "Design loaded.\n";
  else
    std::cerr << "Failed to read verilog.\n";
}

static void cmd_write_verilog( CLIContext& ctx, const std::vector<std::string>& args )
{
  if ( !ctx.ntk )
  {
    std::cerr << "Error: load a network first with read_verilog.\n";
    return;
  }
  if ( args.size() < 2 )
  {
    std::cerr << "Usage: write_verilog <filename>\n";
    return;
  }
  std::ofstream out( args[1] );
  if ( !out )
  {
    std::cerr << "Cannot write to " << args[1] << "\n";
    return;
  }
  rinox::io::verilog::write_verilog( *ctx.ntk, out );
  std::cout << "Design written to " << args[1] << "\n";
}

std::map<std::string, CommandHandler> register_verilog_commands()
{
  return {
      { "read_verilog", cmd_read_verilog },
      { "write_verilog", cmd_write_verilog },
  };
}
