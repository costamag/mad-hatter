#pragma once
#include <map>
#include <string>
#include <vector>
#include "context.hpp"

using CommandHandler = void(*)(CLIContext&, const std::vector<std::string>&);

std::map<std::string, CommandHandler> register_commands();
