#include "io.hpp"
#include "../../context.hpp"

std::map<std::string, CommandHandler> register_io_commands()
{
  auto cmds = register_verilog_commands();
  auto json_cmds = register_json_commands();
  cmds.insert( json_cmds.begin(), json_cmds.end() );
  auto stats_cmds = register_stats_commands();
  cmds.insert( stats_cmds.begin(), stats_cmds.end() );
  return cmds;
}
