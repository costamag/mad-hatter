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
#include <rinox/io/json/json.hpp>
#include <rinox/io/json/json_parser.hpp>
#include <rinox/io/json/json_stream.hpp>
#include <rinox/io/utils/reader.hpp>
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

  bound_network ntk( gates );
  std::istringstream in_ntk2( file );
  const auto result_ntk = rinox::io::json::read_json( in_ntk2, rinox::io::reader( ntk ) );
  /* structural checks */
  CHECK( result_ntk == lorina::return_code::success );
  CHECK( ntk.num_pis() == 2 );
  CHECK( ntk.num_pos() == 1 );
  CHECK( ntk.size() == 6 );
  CHECK( ntk.num_gates() == 2 );

  auto const a = ntk.make_signal( ntk.pi_at( 0 ) );
  auto const b = ntk.make_signal( ntk.pi_at( 1 ) );
  auto const f1 = ntk.create_node<true>( { a, b }, 2u );
  CHECK( f1.index == 4u );
  auto const f2 = ntk.create_node<true>( { f1 }, 0u );
  CHECK( f2.index == 5u );

  CHECK( ntk.num_pis() == 2 );
  CHECK( ntk.num_pos() == 1 );
  CHECK( ntk.size() == 6 );
  CHECK( ntk.num_gates() == 2 );
}

TEST_CASE( "Read json to mapped network with multiple output gates", "[json_parsing]" )
{

  using bound_network = rinox::network::bound_network<rinox::network::design_type_t::CELL_BASED, 2>;
  std::vector<mockturtle::gate> gates;

  std::istringstream in_lib( test_library );
  auto result_lib = lorina::read_genlib( in_lib, genlib_reader( gates ) );
  CHECK( result_lib == lorina::return_code::success );

  std::string file = R"({
  "creator": "Yosys 0.30",
  "modules": {
    "top": {
      "attributes": {},
      "ports": {
        "a":   { "direction": "input",  "bits": [ 2 ] },
        "b":   { "direction": "input",  "bits": [ 3 ] },
        "cin": { "direction": "input",  "bits": [ 4 ] },
        "s":   { "direction": "output", "bits": [ 9 ] },
        "cout":{ "direction": "output", "bits": [ 8 ] }
      },
      "cells": {
        "buf_s": {
          "hide_name": 0,
          "type": "buf",
          "parameters": {},
          "attributes": {},
          "port_directions": { "a": "input", "O": "output" },
          "connections": { "a": [ 7 ], "O": [ 9 ] }
        },
        "buf_c": {
          "hide_name": 0,
          "type": "buf",
          "parameters": {},
          "attributes": {},
          "port_directions": { "a": "input", "O": "output" },
          "connections": { "a": [ 6 ], "O": [ 8 ] }
        },
        "fa0": {
          "hide_name": 0,
          "type": "fa",
          "parameters": {},
          "attributes": {},
          "port_directions": {
            "a": "input", "b": "input", "c": "input",
            "S": "output", "C": "output"
          },
          "connections": {
            "a": [ 2 ],
            "b": [ 3 ],
            "c": [ 4 ],
            "S": [ 7 ],
            "C": [ 6 ]
          }
        }
      },
      "netnames": {
        "a":      { "hide_name": 0, "bits": [ 2 ] },
        "b":      { "hide_name": 0, "bits": [ 3 ] },
        "cin":    { "hide_name": 0, "bits": [ 4 ] },
        "n_carry":{ "hide_name": 1, "bits": [ 6 ] },
        "n_sum":  { "hide_name": 1, "bits": [ 7 ] },
        "cout":   { "hide_name": 0, "bits": [ 8 ] },
        "s":      { "hide_name": 0, "bits": [ 9 ] }
      }
    }
  }
})";

  bound_network ntk( gates );
  using signal = bound_network::signal;
  std::istringstream in_ntk( file );
  const auto result_ntk = rinox::io::json::read_json( in_ntk, rinox::io::reader( ntk ) );
  /* structural checks */
  CHECK( result_ntk == lorina::return_code::success );
  CHECK( ntk.num_pis() == 3 );
  CHECK( ntk.num_pos() == 2 );
  CHECK( ntk.size() == 8 );
  CHECK( ntk.num_gates() == 3 );

  auto const a = ntk.make_signal( ntk.pi_at( 0 ) );
  auto const b = ntk.make_signal( ntk.pi_at( 1 ) );
  auto const c = ntk.make_signal( ntk.pi_at( 2 ) );
  auto const f1 = ntk.create_node<true>( { a, b, c }, { 12u, 13u } );
  CHECK( f1.index == 5u );
  auto const f2 = ntk.create_node<true>( { signal{ f1.index, 0 } }, 7u );
  CHECK( f2.index == 6u );
  auto const f3 = ntk.create_node<true>( { signal{ f1.index, 1 } }, 7u );
  CHECK( f3.index == 7u );

  CHECK( ntk.num_pis() == 3 );
  CHECK( ntk.num_pos() == 2 );
  CHECK( ntk.size() == 8 );
  CHECK( ntk.num_gates() == 3 );
}

TEST_CASE( "Read json to mapped network with multiple output gates and constants", "[json_parsing]" )
{

  using bound_network = rinox::network::bound_network<rinox::network::design_type_t::CELL_BASED, 2>;
  std::vector<mockturtle::gate> gates;

  std::istringstream in_lib( test_library );
  auto result_lib = lorina::read_genlib( in_lib, genlib_reader( gates ) );
  CHECK( result_lib == lorina::return_code::success );

  std::string file = R"({
  "creator": "Yosys 0.30",
  "modules": {
    "top": {
      "attributes": {},
      "ports": {
        "a":   { "direction": "input",  "bits": [ 2 ] },
        "b":   { "direction": "input",  "bits": [ 3 ] },
        "din": { "direction": "input",  "bits": [ 20, "1", 22, "0" ], "upto": 1 },
        "dout":{ "direction": "output", "bits": [ "1", 30, 31, "0" ], "upto": 0 }
      },

      "cells": {
        "buf_to_dout_bit2": {
          "hide_name": 0,
          "type": "buf",
          "parameters": {},
          "attributes": {},
          "port_directions": { "a": "input", "O": "output" },
          "connections": { "a": [ 15 ], "O": [ 31 ] }
        },

        "and_const00": {
          "hide_name": 0,
          "type": "and2",
          "parameters": {},
          "attributes": {},
          "port_directions": { "a": "input", "b": "input", "O": "output" },
          "connections": { "a": [ "0" ], "b": [ "0" ], "O": [ 14 ] }
        },

        "xor_from_din": {
          "hide_name": 0,
          "type": "xor2",
          "parameters": {},
          "attributes": {},
          "port_directions": { "a": "input", "b": "input", "O": "output" },
          "connections": { "a": [ 20 ], "b": [ 22 ], "O": [ 15 ] }
        },

        "inv_a_to_dout_bit1": {
          "hide_name": 0,
          "type": "inv1",
          "parameters": {},
          "attributes": {},
          "port_directions": { "a": "input", "O": "output" },
          "connections": { "a": [ 2 ], "O": [ 30 ] }
        }
      },

      "netnames": {
        "a":     { "hide_name": 0, "bits": [ 2 ] },
        "b":     { "hide_name": 0, "bits": [ 3 ] },

        "din":   { "hide_name": 0, "bits": [ 20, "1", 22, "0" ], "upto": 1 },
        "dout":  { "hide_name": 0, "bits": [ "1", 30, 31, "0" ], "upto": 0 },

        "n_and00": { "hide_name": 1, "bits": [ 14 ] },
        "n_xor":   { "hide_name": 1, "bits": [ 15 ] }
      }
    }
  }
})";

  bound_network ntk( gates );
  using signal = bound_network::signal;
  std::istringstream in_ntk( file );
  const auto result_ntk = rinox::io::json::read_json( in_ntk, rinox::io::reader( ntk ) );
  /* structural checks */
  CHECK( result_ntk == lorina::return_code::success );
  CHECK( ntk.num_pis() == 4 );
  CHECK( ntk.num_pos() == 4 );
  CHECK( ntk.size() == 10 );
  CHECK( ntk.num_gates() == 4 );
  std::unordered_map<std::string, signal> fs;
  fs["0"] = ntk.make_signal( 0 );
  fs["1"] = ntk.make_signal( 1 );
  fs["2"] = ntk.make_signal( ntk.pi_at( 0 ) );
  fs["3"] = ntk.make_signal( ntk.pi_at( 1 ) );
  fs["20"] = ntk.make_signal( ntk.pi_at( 2 ) );
  fs["22"] = ntk.make_signal( ntk.pi_at( 3 ) );
  fs["14"] = ntk.create_node<true>( { fs["0"], fs["0"] }, 3u );
  fs["15"] = ntk.create_node<true>( { fs["20"], fs["22"] }, 4u );
  fs["30"] = ntk.create_node<true>( { fs["2"] }, 0u );
  fs["31"] = ntk.create_node<true>( { fs["15"] }, 7u );
  CHECK( ntk.po_at( 0 ) == fs["1"] );
  CHECK( ntk.po_at( 1 ) == fs["30"] );
  CHECK( ntk.po_at( 2 ) == fs["31"] );
  CHECK( ntk.po_at( 3 ) == fs["0"] );
  CHECK( ntk.num_pis() == 4 );
  CHECK( ntk.num_pos() == 4 );
  CHECK( ntk.size() == 10 );
  CHECK( ntk.num_gates() == 4 );
}