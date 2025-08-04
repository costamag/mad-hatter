#pragma once
#include <map>
#include <string>
#include "../../commands.hpp"   // adjust path for CommandHandler

std::map<std::string, CommandHandler> register_verilog_commands();
