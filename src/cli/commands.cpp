#include "commands.hpp"
#include "commands/libraries.hpp"
#include "commands/io/verilog.hpp"

std::map<std::string, CommandHandler> register_commands() {
  auto cmds = register_library_commands();
  auto verilog_cmds = register_verilog_commands();
  cmds.insert(verilog_cmds.begin(), verilog_cmds.end());
  return cmds;
}
