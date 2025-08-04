#pragma once
#include <functional>
#include <map>
#include <string>
#include <vector>

// Forward declare context
struct CLIContext;

using CommandHandler = std::function<void( CLIContext&, const std::vector<std::string>& )>;

std::map<std::string, CommandHandler> register_commands();