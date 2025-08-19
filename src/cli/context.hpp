#pragma once

#include "../../include/rinox/databases/mapped_database.hpp"
#include "../../include/rinox/network/network.hpp"
#include <mockturtle/io/genlib_reader.hpp>
#include <optional>
#include <vector>

struct CLIContext
{
  static constexpr rinox::network::design_type_t CellType = rinox::network::design_type_t::CELL_BASED;
  using CellNtk = rinox::network::bound_network<CellType, 2>;
  std::vector<mockturtle::gate> gates;
  std::optional<CellNtk> ntk;
  std::optional<rinox::databases::mapped_database<CellNtk, 4>> db4;
};