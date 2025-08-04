#include "commands.hpp"
#include "commands/io/io.hpp"
#include "commands/libraries.hpp"

std::map<std::string, CommandHandler> register_commands()
{
  auto cmds = register_library_commands();
  auto io_cmds = register_io_commands();
  cmds.insert( io_cmds.begin(), io_cmds.end() );
  return cmds;
}
