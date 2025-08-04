#pragma once
#include <map>
#include <string>
#include "../commands.hpp"   // for CommandHandler

std::map<std::string, CommandHandler> register_library_commands();