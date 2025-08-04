#include "stats.hpp"
#include "../../../../include/mad_hatter/network/network.hpp"
#include "../../context.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <lorina/verilog.hpp>

void print_stats( const Stats& stats, const std::string& format )
{
  if ( format == "table" )
  {
    std::cout << std::left
              << std::setw( 10 ) << "Inputs" << ": " << stats.inputs << "\n"
              << std::setw( 10 ) << "Outputs" << ": " << stats.outputs << "\n"
              << std::setw( 10 ) << "Latency" << ": " << stats.latency << "\n"
              << std::setw( 10 ) << "Nodes" << ": " << stats.nodes << "\n"
              << std::setw( 10 ) << "Edges" << ": " << stats.edges << "\n"
              << std::setw( 10 ) << "Area" << ": " << std::fixed << std::setprecision( 2 ) << stats.area << "\n"
              << std::setw( 10 ) << "Delay" << ": " << stats.delay << "\n"
              << std::setw( 10 ) << "Levels" << ": " << stats.levels << "\n";
  }
  else if ( format == "grep" )
  {
    std::cout << "Inputs=" << stats.inputs
              << " Outputs=" << stats.outputs
              << " Latency=" << stats.latency
              << " Nodes=" << stats.nodes
              << " Edges=" << stats.edges
              << " Area=" << std::fixed << std::setprecision( 2 ) << stats.area
              << " Delay=" << stats.delay
              << " Levels=" << stats.levels
              << "\n";
  }
  else if ( format == "json" )
  {
    std::cout << "{"
              << "\"inputs\":" << stats.inputs << ","
              << "\"outputs\":" << stats.outputs << ","
              << "\"latency\":" << stats.latency << ","
              << "\"nodes\":" << stats.nodes << ","
              << "\"edges\":" << stats.edges << ","
              << "\"area\":" << std::fixed << std::setprecision( 2 ) << stats.area << ","
              << "\"delay\":" << stats.delay << ","
              << "\"levels\":" << stats.levels
              << "}\n";
  }
  else
  {
    std::cerr << "Unknown format: " << format << "\n";
  }
}

static void cmd_print_stats( CLIContext& ctx, const std::vector<std::string>& args )
{
  if ( !ctx.ntk )
  {
    std::cerr << "Error: load a network first with read_verilog.\n";
    return;
  }

  // Default format
  std::string format = "table";
  if ( args.size() >= 2 )
  {
    format = args[1]; // user can do: print_stats json
  }

  // collect stats from the network
  Stats stats;
  stats.inputs = ctx.ntk->num_pis();  // primary inputs
  stats.outputs = ctx.ntk->num_pos(); // primary outputs
  stats.latency = 0;                  // placeholder unless you track latency
  stats.nodes = ctx.ntk->num_gates(); // total nodes
  stats.edges = 0;                    // adjust if you have an API for edges
  stats.levels = 0;                   // logic depth
  stats.area = ctx.ntk->area();       // TODO: compute from ctx.gates if available
  stats.delay = 0.0;                  // TODO: compute from timing analysis

  print_stats( stats, format );
}

std::map<std::string, CommandHandler> register_stats_commands()
{
  return {
      { "print_stats", cmd_print_stats } };
}