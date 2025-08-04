#include "cli_repl.hpp"
#include <sstream>

static std::vector<std::string> split( const std::string& line )
{
  std::istringstream iss( line );
  std::vector<std::string> tokens;
  std::string token;
  while ( iss >> token )
    tokens.push_back( token );
  return tokens;
}

static bool execute_command( CLIContext& ctx,
                             const std::map<std::string, CommandHandler>& commands,
                             const std::vector<std::string>& tokens,
                             std::ostream& out )
{
  if ( tokens.empty() )
    return true;
  auto it = commands.find( tokens[0] );
  if ( it != commands.end() )
  {
    it->second( ctx, tokens );
    return true;
  }
  if ( tokens[0] == "quit" || tokens[0] == "exit" )
    return false;
  out << "Unknown command: " << tokens[0] << "\n";
  return true;
}

void run_repl( CLIContext& ctx,
               std::istream& in,
               std::ostream& out,
               const std::map<std::string, CommandHandler>& commands )
{
  std::string line;
  while ( std::getline( in, line ) )
  {
    if ( !execute_command( ctx, commands, split( line ), out ) )
      break;
  }
}
