#include "io.hpp"
#include "../../context.hpp"

std::map<std::string, CommandHandler> register_io_commands()
{
  auto cmds = register_verilog_commands();
  auto stats_cmds = register_stats_commands();
  cmds.insert( stats_cmds.begin(), stats_cmds.end() );
  return cmds;
}
