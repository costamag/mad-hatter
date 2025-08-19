#pragma once
#include <map>
#include <string>
#include <rinox/databases/databases.hpp>
#include "../../commands.hpp"   // for CommandHandler

std::map<std::string, CommandHandler> register_db_commands();