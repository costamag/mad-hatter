#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <vector>

#include <lorina/genlib.hpp>
#include <rinox/network/network.hpp>
#include <rinox/analyzers/evaluators/power_evaluator.hpp>
#include <mockturtle/io/genlib_reader.hpp>
#include <mockturtle/io/super_reader.hpp>
// Update the include path to the correct location of power_evaluator.hpp
#include <mockturtle/utils/tech_library.hpp>

using namespace mockturtle;
using namespace rinox;
using namespace network;
using namespace analyzers::evaluators;

std::string const test_library = "GATE   inv1    1 O=!a;            PIN * INV 1 999 0.9 0.3 0.9 0.3\n"
                                 "GATE   inv2    2 O=!a;            PIN * INV 2 999 1.0 0.1 1.0 0.1\n"
                                 "GATE   nor2    2 O=!(a+b);        PIN * INV 1 999 1.0 0.2 1.0 0.2\n"
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

TEST_CASE( "Power evaluation in Bound networks", "[power_evaluator]" )
{
  using Ntk = network::bound_network<network::design_type_t::CELL_BASED, 2>;
  std::vector<gate> gates;

  std::istringstream in( test_library );
  auto result = lorina::read_genlib( in, genlib_reader( gates ) );
  CHECK( result == lorina::return_code::success );

  Ntk ntk( gates );
  auto const a = ntk.create_pi();
  auto const b = ntk.create_pi();
  /* chain of inverters */
  auto const f1 = ntk.create_node( { a, b }, 2 );
  auto const f2 = ntk.create_node( { a, f1 }, 2 );
  ntk.create_po( f2 );

  using TT = kitty::partial_truth_table;
  static constexpr uint32_t num_steps = 10;

  std::vector<TT> tts_init;
  tts_init.emplace_back( 1 );
  tts_init.emplace_back( 1 );
  std::vector<TT> tts_end;
  tts_end.emplace_back( 1 );
  tts_end.emplace_back( 1 );
  kitty::create_from_binary_string( tts_init[0], "1" );
  kitty::create_from_binary_string( tts_init[1], "0" );
  kitty::create_from_binary_string( tts_end[0], "0" );
  kitty::create_from_binary_string( tts_end[1], "0" );

  analyzers::utils::workload<TT, num_steps> work( tts_init, tts_end );

  power_evaluator_stats st;
  power_evaluator<Ntk, TT, num_steps> power( ntk, st );

  power.run( work );

  CHECK( st.glitching == 2.0 );
  CHECK( st.switching == 3 );
  CHECK( st.dyn_power == 3.0 );
  auto sim_eval = power.to_string();
  auto const sim_res = power.to_string();
  auto const sim_exp = "2 0 -----_____ \n"
                       "3 0 __________ \n"
                       "4 0 _______--- \n"
                       "5 0 ___-----__ \n";
  CHECK( sim_res == sim_exp );
}
