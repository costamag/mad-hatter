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
  const auto result_ntk = rinox::io::json::json_stream( in_ntk );
}
