#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <sstream>
#include <string>

#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/networks/buffered.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/muxig.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/networks/xmg.hpp>
#include <rinox/io/json/json_stream.hpp>
#include <rinox/network/network.hpp>

#include <kitty/kitty.hpp>

using namespace mockturtle;

std::string const test_library = "GATE   inv1    1 O=!a;            PIN * INV 1 999 0.9 0.3 0.9 0.3\n"
                                 "GATE   inv2    2 O=!a;            PIN * INV 2 999 1.0 0.1 1.0 0.1\n"
                                 "GATE   nand2   2 O=!(a*b);        PIN * INV 1 999 1.0 0.2 1.0 0.2\n"
                                 "GATE   and2    3 O=a*b;           PIN * INV 1 999 1.7 0.2 1.7 0.2\n"
                                 "GATE   xor2    4 O=a^b;           PIN * UNKNOWN 2 999 1.9 0.5 1.9 0.5\n"
                                 "GATE   mig3    3 O=a*b+a*c+b*c;   PIN * INV 1 999 2.0 0.2 2.0 0.2\n"
                                 "GATE   xor3    5 O=a^b^c;         PIN * UNKNOWN 2 999 3.0 0.5 3.0 0.5\n"
                                 "GATE   buf     2 O=a;             PIN * NONINV 1 999 1.0 0.0 1.0 0.0\n"
                                 "GATE   zero    0 O=CONST0;\n"
                                 "GATE   one     0 O=CONST1;\n"
                                 "GATE   ha      5 C=a*b;           PIN * INV 1 999 1.7 0.4 1.7 0.4\n"
                                 "GATE   ha      5 S=!a*b+a*!b;     PIN * INV 1 999 2.1 0.4 2.1 0.4\n"
                                 "GATE   fa      6 C=a*b+a*c+b*c;   PIN * INV 1 999 2.1 0.4 2.1 0.4\n"
                                 "GATE   fa      6 S=a^b^c;         PIN * INV 1 999 3.0 0.4 3.0 0.4";

TEST_CASE( "Read structural json to mapped network", "[json_parsing]" )
{

  using bound_network = rinox::network::bound_network<rinox::network::design_type_t::CELL_BASED, 2>;
  std::vector<mockturtle::gate> gates;

  std::istringstream in_lib( test_library );
  auto result_lib = lorina::read_genlib( in_lib, genlib_reader( gates ) );
  CHECK( result_lib == lorina::return_code::success );

  std::string file =
      R"({
"creator": "Yosys 0.30",
  "modules": {
    "top": {
      "attributes": {},
      "ports": {
        "a": {
          "direction": "input",
          "bits": [ 2 ]
        },
        "b": {
          "direction": "input",
          "bits": [ 3 ]
        },
        "y": {
          "direction": "output",
          "bits": [ 5 ]
        }
      },
      "cells": {
        "nand0": {
          "hide_name": 0,
          "type": "nand2",
          "parameters": {},
          "attributes": {},
          "port_directions": {
            "a": "input",
            "b": "input",
            "O": "output"
          },
          "connections": {
            "a": [ 2 ],
            "b": [ 3 ],
            "O": [ 4 ]
          }
        },
        "inv0": {
          "hide_name": 0,
          "type": "inv1",
          "parameters": {},
          "attributes": {},
          "port_directions": {
            "a": "input",
            "O": "output"
          },
          "connections": {
            "a": [ 4 ],
            "O": [ 5 ]
          }
        }
      },
      "netnames": {
        "a": {
          "hide_name": 0,
          "bits": [ 2 ]
        },
        "b": {
          "hide_name": 0,
          "bits": [ 3 ]
        },
        "n1": {
          "hide_name": 1,
          "bits": [ 4 ]
        },
        "y": {
          "hide_name": 0,
          "bits": [ 5 ]
        }
      }
    }
  }
})";

  std::istringstream in_ntk( file );

  bound_network ntk( gates );
  rinox::io::json::json_stream jstream( in_ntk );
  rinox::io::json::instance_t inst;
  rinox::io::json::instance_return_code code;

  std::vector<std::string> ports_names{ "a", "b", "y" };
  std::vector<std::string> ports_dirs{ "input", "input", "output" };
  std::vector<int64_t> ports_bits{ 2, 3, 5 };

  for ( auto i = 0u; i < 3u; ++i )
  {
    code = jstream.get_instance( inst );
    CHECK( code == rinox::io::json::instance_return_code::valid );
    CHECK( std::holds_alternative<rinox::io::json::port_instance_t>( inst ) );
    auto const* p = std::get_if<rinox::io::json::port_instance_t>( &inst );
    REQUIRE( p != nullptr );
    CHECK( p->name == ports_names[i] );
    CHECK( p->direction == ports_dirs[i] );
    CHECK( p->bits.size() == 1 );
    CHECK( std::holds_alternative<int64_t>( p->bits[0].v ) );
    CHECK( std::get<int64_t>( p->bits[0].v ) == ports_bits[i] );
  }

  std::vector<std::string> cell_name{ "nand0", "inv0" };
  ;
  std::vector<std::string> cell_type{ "nand2", "inv1" };
  std::vector<std::string> cell_model;
  std::vector<std::unordered_map<std::string, std::string>> cell_port_dirs( 2u );
  cell_port_dirs[0]["a"] = "input";
  cell_port_dirs[0]["b"] = "input";
  cell_port_dirs[0]["O"] = "output";
  cell_port_dirs[1]["a"] = "input";
  cell_port_dirs[1]["O"] = "output";
  std::vector<std::unordered_map<std::string, std::vector<rinox::io::json::json_bit_t>>> cell_connections( 2u );
  cell_connections[0]["a"] = { rinox::io::json::json_bit_t{ 2 } };
  cell_connections[0]["b"] = { rinox::io::json::json_bit_t{ 3 } };
  cell_connections[0]["O"] = { rinox::io::json::json_bit_t{ 4 } };
  cell_connections[1]["a"] = { rinox::io::json::json_bit_t{ 4 } };
  cell_connections[1]["O"] = { rinox::io::json::json_bit_t{ 5 } };
  std::vector<std::vector<std::pair<std::string, std::string>>> cell_parameters{ {}, {} };
  std::vector<std::vector<std::pair<std::string, std::string>>> cell_attributes{ {}, {} };
  for ( auto i = 0u; i < 2u; ++i )
  {
    code = jstream.get_instance( inst );
    CHECK( code == rinox::io::json::instance_return_code::valid );
    CHECK( std::holds_alternative<rinox::io::json::cell_instance_t>( inst ) );
    auto const* p = std::get_if<rinox::io::json::cell_instance_t>( &inst );
    REQUIRE( p != nullptr );
    CHECK( p->name == cell_name[i] );
    CHECK( p->type == cell_type[i] );
    CHECK( p->port_dirs == cell_port_dirs[i] );
    CHECK( p->connections == cell_connections[i] );
  }

  std::vector<std::string> net_name{ "a", "b", "n1", "y" };
  std::vector<std::vector<rinox::io::json::json_bit_t>> net_bits;
  for ( int64_t bit = 2; bit < 6; ++bit )
    net_bits.push_back( std::vector<rinox::io::json::json_bit_t>{ rinox::io::json::json_bit_t{ bit } } );

  for ( auto i = 0u; i < 4u; ++i )
  {
    code = jstream.get_instance( inst );
    CHECK( code == rinox::io::json::instance_return_code::valid );
    CHECK( std::holds_alternative<rinox::io::json::net_name_instance_t>( inst ) );
    auto const* p = std::get_if<rinox::io::json::net_name_instance_t>( &inst );
    REQUIRE( p != nullptr );
    CHECK( p->name == net_name[i] );
    CHECK( p->bits == net_bits[i] );
  }
  code = jstream.get_instance( inst );
  CHECK( code == rinox::io::json::instance_return_code::end );
}