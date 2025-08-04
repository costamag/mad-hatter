#pragma once

#include "../include/mad_hatter/network/network.hpp"
#include <mockturtle/io/genlib_reader.hpp>
#include <optional>
#include <vector>

struct CLIContext
{
  std::vector<mockturtle::gate> gates;
  std::optional<
      mad_hatter::network::bound_network<
          mad_hatter::network::design_type_t::CELL_BASED, 2>>
      ntk;
};