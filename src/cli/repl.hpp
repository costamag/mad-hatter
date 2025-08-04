#pragma once
#include "commands.hpp"
#include "context.hpp"
#include <iostream>

void run_repl( CLIContext& ctx,
               std::istream& in,
               std::ostream& out,
               const std::map<std::string, CommandHandler>& commands );
