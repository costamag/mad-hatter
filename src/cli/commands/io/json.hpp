#pragma once
#include "../../commands.hpp" // adjust path for CommandHandler
#include <map>
#include <string>

std::map<std::string, CommandHandler> register_json_commands();
