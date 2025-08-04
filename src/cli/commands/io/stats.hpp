#pragma once
#include "../../commands.hpp" // adjust path for CommandHandler
#include <map>
#include <string>

// simple struct to hold stats data
struct Stats
{
  int inputs = 0;
  int outputs = 0;
  int latency = 0;
  int nodes = 0;
  int edges = 0;
  double area = 0.0;
  double delay = 0.0;
  int levels = 0;
};

// function declaration
void print_stats( const Stats& stats, const std::string& format = "table" );

std::map<std::string, CommandHandler> register_stats_commands();
