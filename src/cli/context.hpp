#pragma once

#include "../../include/rinox/network/network.hpp"
#include <mockturtle/io/genlib_reader.hpp>
#include <optional>
#include <vector>

struct CLIContext
{
  std::vector<mockturtle::gate> gates;
  std::optional<
      rinox::network::bound_network<
          rinox::network::design_type_t::CELL_BASED, 2>>
      ntk;
};