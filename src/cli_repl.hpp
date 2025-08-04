#pragma once
#include "cli_commands.hpp"
#include "cli_context.hpp"
#include <iostream>

void run_repl( CLIContext& ctx,
               std::istream& in,
               std::ostream& out,
               const std::map<std::string, CommandHandler>& commands );
