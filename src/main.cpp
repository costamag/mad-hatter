#include "cli_commands.hpp"
#include "cli_context.hpp"
#include "cli_repl.hpp"
#include <CLI/CLI.hpp>
#include <fstream>
#include <iostream>
#include <replxx.hxx>
#include <sstream>

int main( int argc, char* argv[] )
{
  CLIContext ctx;
  auto commands = register_commands();

  CLI::App app{ "mad — Technology Mapping CLI" };

  std::string libFile, verilogIn, verilogOut;
  bool forceNonInteractive = false;
  std::string scriptFile;

  app.add_option( "-l,--lib", libFile, "Technology library (.lib)" );
  app.add_option( "-i,--input", verilogIn, "Input Verilog design" );
  app.add_option( "-o,--output", verilogOut, "Output Verilog design" );
  app.add_option( "-f,--file", scriptFile, "Script file with commands" );
  app.add_flag( "--no-repl", forceNonInteractive, "Run in non-interactive mode only" );

  CLI11_PARSE( app, argc, argv );

  // batch mode via flags
  if ( !libFile.empty() || !verilogIn.empty() || !verilogOut.empty() )
  {
    if ( !libFile.empty() )
      commands["read_genlib"]( ctx, { "read_genlib", libFile } );
    if ( !verilogIn.empty() )
      commands["read_verilog"]( ctx, { "read_verilog", verilogIn } );
    if ( !verilogOut.empty() )
      commands["write_verilog"]( ctx, { "write_verilog", verilogOut } );
    return 0;
  }

  // script mode
  if ( !scriptFile.empty() )
  {
    std::ifstream script( scriptFile );
    if ( !script )
    {
      std::cerr << "Cannot open script file: " << scriptFile << "\n";
      return 1;
    }
    run_repl( ctx, script, std::cout, commands );
    return 0;
  }

  // interactive REPL mode (default)
  if ( !forceNonInteractive )
  {
    replxx::Replxx repl;
    std::string prompt = "mad> ";

    repl.set_completion_callback( [&]( std::string const& prefix, int& contextLen ) {
      replxx::Replxx::completions_t completions;
      for ( auto& [cmd, _] : commands )
      {
        if ( cmd.find( prefix ) == 0 )
          completions.emplace_back( cmd );
      }
      contextLen = prefix.size();
      return completions;
    } );

    while ( true )
    {
      char const* input = repl.input( prompt.c_str() );
      if ( !input )
        break;
      std::string line( input );
      repl.history_add( line );
      std::istringstream iss( line );
      std::vector<std::string> tokens;
      std::string token;
      while ( iss >> token )
        tokens.push_back( token );
      if ( tokens.empty() )
        continue;
      if ( tokens[0] == "quit" || tokens[0] == "exit" )
        break;
      auto it = commands.find( tokens[0] );
      if ( it != commands.end() )
        it->second( ctx, tokens );
      else
        std::cout << "Unknown command: " << tokens[0] << "\n";
    }
    return 0;
  }

  // no inputs, REPL disabled
  std::cerr << "Error: No commands provided and --no-repl set.\n";
  return 2;
}
