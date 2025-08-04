#include "libraries.hpp"
#include "../context.hpp"
#include <fstream>
#include <iostream>
#include <lorina/genlib.hpp>

static void cmd_read_genlib(CLIContext& ctx, const std::vector<std::string>& args) {
  if (args.size() < 2) {
    std::cerr << "Usage: read_genlib <filename>\n";
    return;
  }
  std::ifstream in_lib(args[1]);
  if (!in_lib) {
    std::cerr << "Cannot open " << args[1] << "\n";
    return;
  }
  auto result_lib = lorina::read_genlib(in_lib, mockturtle::genlib_reader(ctx.gates));
  if (result_lib == lorina::return_code::success)
    std::cout << "Library loaded.\n";
  else
    std::cerr << "Failed to read library.\n";
}

std::map<std::string, CommandHandler> register_library_commands() {
  return {
      {"read_genlib", cmd_read_genlib},
  };
}
