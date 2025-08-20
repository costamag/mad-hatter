#pragma once

#include "../../network/network.hpp"
#include "../../network/signal_map.hpp"
#include "../../traits.hpp"

#include <mockturtle/traits.hpp>
#include <mockturtle/views/topo_view.hpp>
#include <nlohmann/json.hpp>

#include <fstream>
#include <string>
#include <unordered_map>

#pragma once

#include "../../network/network.hpp"
#include "../../network/signal_map.hpp"
#include "../../traits.hpp"

#include <mockturtle/traits.hpp>
#include <mockturtle/views/topo_view.hpp>
#include <nlohmann/json.hpp>

#include <fstream>
#include <string>
#include <unordered_map>

#pragma once

#include "../../network/network.hpp"
#include "../../network/signal_map.hpp"
#include "../../traits.hpp"

#include <mockturtle/traits.hpp>
#include <mockturtle/views/topo_view.hpp>
#include <nlohmann/json.hpp>

#include <fstream>
#include <string>
#include <unordered_map>

namespace rinox
{

namespace io
{

namespace json
{

using json = nlohmann::json;

template<network::design_type_t DesignStyle, uint32_t MaxNumOutputs>
void write_json( network::bound_network<DesignStyle, MaxNumOutputs> const& ntk, std::ostream& os )
{
  using Ntk = network::bound_network<DesignStyle, MaxNumOutputs>;
  mockturtle::topo_view topo_ntk{ ntk };
  auto const& gates = ntk.get_library();

  json j = json::object();
  j["creator"] = "Rinox";

  std::string module_name = "top";
  json module = json::object();
  module["attributes"] = json::object();

  // Ports
  json ports = json::object();
  std::unordered_map<std::string, uint32_t> port_to_index;
  ntk.foreach_pi( [&]( auto const& n, uint32_t i ) {
    std::string name = ntk.has_name( ntk.make_signal( n ) ) ? ntk.get_name( ntk.make_signal( n ) ) : fmt::format( "x{}", i );
    json bits = json::array();
    bits.push_back( i );
    ports[name] = json::object( { { "direction", "input" },
                                  { "bits", bits } } );
    port_to_index[name] = i;
  } );
  ntk.foreach_po( [&]( auto const& f, uint32_t i ) {
    std::string name = ntk.has_output_name( i ) ? ntk.get_output_name( i ) : fmt::format( "y{}", i );

    uint32_t idx = ntk.node_to_index( ntk.get_node( f ) );
    json bits = json::array();
    bits.push_back( idx );
    ports[name] = json::object( { { "direction", "output" },
                                  { "bits", bits } } );
    port_to_index[name] = idx;
  } );
  module["ports"] = ports;

  // Cells
  json cells = json::object();
  uint32_t cell_counter = 0;

  topo_ntk.foreach_node( [&]( auto const& n ) {
    if ( ntk.has_binding( n ) )
    {
      const auto& gate = gates[ntk.get_binding_index( n )];
      std::string base_type = gate.name;
      std::string inst_name = fmt::format( "{}{}", base_type, cell_counter++ ); // e.g., "nand0"

      json connections = json::object();
      json port_directions = json::object();

      // Inputs
      ntk.foreach_fanin( n, [&]( auto const& f, uint32_t i ) {
        std::string port = gate.pins[i].name;
        json bit_array = json::array();
        if ( ntk.is_constant( ntk.get_node( f ) ) )
        {
          bit_array.push_back( ntk.is_complemented( f ) ? "0" : "1" );
        }
        else
        {
          bit_array.push_back( ntk.node_to_index( ntk.get_node( f ) ) );
        }
        connections[port] = bit_array;
        port_directions[port] = "input";
      } );

      // Outputs
      ntk.foreach_output( n, [&]( auto const& f ) {
        std::string out_port = gate.output_name;
        json bit_array = json::array();
        bit_array.push_back( f.index );
        connections[out_port] = bit_array;
        port_directions[out_port] = "output";
      } );

      json cell = json::object();
      cell["hide_name"] = 0;
      cell["type"] = base_type;
      cell["parameters"] = json::object();
      cell["attributes"] = json::object();
      cell["port_directions"] = port_directions;
      cell["connections"] = connections;

      cells[inst_name] = cell;
    }
  } );

  module["cells"] = cells;

  // Netnames
  json netnames = json::object();
  std::unordered_map<uint32_t, std::string> index_to_name;

  // PIs
  ntk.foreach_pi( [&]( auto const& n, uint32_t i ) {
    std::string name = ntk.has_name( ntk.make_signal( n ) ) ? ntk.get_name( ntk.make_signal( n ) ) : fmt::format( "x{}", i );
    json bits = json::array();
    bits.push_back( i );
    netnames[name] = json::object( { { "hide_name", 0 },
                                     { "bits", bits } } );
    index_to_name[i] = name;
  } );

  // POs
  ntk.foreach_po( [&]( auto const& f, uint32_t i ) {
    std::string name = ntk.has_output_name( i ) ? ntk.get_output_name( i ) : fmt::format( "y{}", i );
    uint32_t idx = ntk.node_to_index( ntk.get_node( f ) );
    json bits = json::array();
    bits.push_back( idx );
    netnames[name] = json::object( { { "hide_name", 0 },
                                     { "bits", bits } } );
    index_to_name[idx] = name;
  } );

  // Internal nets
  ntk.foreach_node( [&]( auto const& n ) {
    if ( ntk.is_constant( n ) || ntk.is_pi( n ) )
      return;
    ntk.foreach_output( n, [&]( auto const& f ) {
      uint32_t idx = f.index;
      if ( index_to_name.count( idx ) == 0 )
      {
        std::string name = fmt::format( "n{}", idx );
        json bits = json::array();
        bits.push_back( idx );
        netnames[name] = json::object( { { "hide_name", 1 },
                                         { "bits", bits } } );
        index_to_name[idx] = name;
      }
    } );
  } );

  module["netnames"] = netnames;

  // Final module structure
  json modules = json::object();
  modules[module_name] = module;
  j["modules"] = modules;

  // Write to stream
  os << j.dump( 2 );
}

template<network::design_type_t DesignStyle, uint32_t MaxNumOutputs>
void write_json( network::bound_network<DesignStyle, MaxNumOutputs> const& ntk, std::string const& filename )
{
  std::ofstream os( filename.c_str(), std::ofstream::out );
  write_json<DesignStyle, MaxNumOutputs>( ntk, os );
  os.close();
}

} // namespace json

} // namespace io

} // namespace rinox
