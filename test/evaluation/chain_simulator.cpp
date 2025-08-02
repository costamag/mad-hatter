#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <kitty/print.hpp>
#include <kitty/static_truth_table.hpp>
#include <mad_hatter/evaluation/chain_simulator.hpp>
#include <mad_hatter/evaluation/chains.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/io/genlib_reader.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/xag.hpp>

using namespace mockturtle;
using namespace mad_hatter::evaluation;
using namespace mad_hatter::evaluation::chains;
using namespace mad_hatter::libraries;

TEST_CASE( "simulation of xag_boolean_chain with static truth tables", "[chain_simulator]" )
{
  xag_network xag;
  auto const a = xag.create_pi();
  auto const b = xag.create_pi();
  auto const c = xag.create_pi();
  auto const d = xag.create_pi();
  auto const t0 = xag.create_and( a, b );
  auto const t1 = xag.create_and( c, d );
  auto const t2 = xag.create_xor( t0, t1 );
  xag.create_po( t2 );

  std::vector<kitty::static_truth_table<4u>> xs;
  for ( auto i = 0u; i < 4u; ++i )
  {
    xs.emplace_back();
    kitty::create_nth_var( xs[i], i );
  }
  xs.emplace_back( xs[0] & xs[1] ); // 4
  xs.emplace_back( xs[2] & xs[3] ); // 5
  xs.emplace_back( xs[4] ^ xs[5] ); // 6

  std::vector<kitty::static_truth_table<4u> const*> xs_r;
  for ( auto i = 0u; i < 4u; ++i )
  {
    xs_r.emplace_back( &xs[i] );
  }
  xag_chain<true> chain_separate;
  encode( chain_separate, xag );

  chain_simulator<xag_chain<true>, kitty::static_truth_table<4u>> sim_separate;
  sim_separate( chain_separate, xs_r );
  kitty::static_truth_table<4u> tt4_separate, tt5_separate, tt6_separate;
  sim_separate.get_simulation_inline( tt4_separate, chain_separate, xs_r, 10u );
  sim_separate.get_simulation_inline( tt5_separate, chain_separate, xs_r, 12u );
  sim_separate.get_simulation_inline( tt6_separate, chain_separate, xs_r, 14u );
  CHECK( kitty::equal( xs[4], tt4_separate ) );
  CHECK( kitty::equal( xs[5], tt5_separate ) );
  CHECK( kitty::equal( xs[6], tt6_separate ) );

  xag_chain<false> chain_unified;
  encode( chain_unified, xag );
  chain_simulator<xag_chain<false>, kitty::static_truth_table<4u>> sim_unified;
  sim_unified( chain_unified, xs_r );
  kitty::static_truth_table<4u> tt4_unified, tt5_unified, tt6_unified;
  sim_unified.get_simulation_inline( tt4_unified, chain_unified, xs_r, 10u );
  sim_unified.get_simulation_inline( tt5_unified, chain_unified, xs_r, 12u );
  sim_unified.get_simulation_inline( tt6_unified, chain_unified, xs_r, 14u );
  CHECK( kitty::equal( xs[4], tt4_unified ) );
  CHECK( kitty::equal( xs[5], tt5_unified ) );
  CHECK( kitty::equal( xs[6], tt6_unified ) );
}

TEST_CASE( "simulation of xag_chain with dynamic truth tables", "[chain_simulator]" )
{
  xag_network xag;
  auto const a = xag.create_pi();
  auto const b = xag.create_pi();
  auto const c = xag.create_pi();
  auto const d = xag.create_pi();
  auto const t0 = xag.create_and( a, b );
  auto const t1 = xag.create_and( c, d );
  auto const t2 = xag.create_xor( t0, t1 );
  xag.create_po( t2 );

  std::vector<kitty::dynamic_truth_table> xs;
  for ( auto i = 0u; i < 4u; ++i )
  {
    xs.emplace_back( 4u );
    kitty::create_nth_var( xs[i], i );
  }
  xs.emplace_back( xs[0] & xs[1] ); // 4
  xs.emplace_back( xs[2] & xs[3] ); // 5
  xs.emplace_back( xs[4] ^ xs[5] ); // 6

  std::vector<kitty::dynamic_truth_table const*> xs_r;
  for ( auto i = 0u; i < 4u; ++i )
  {
    xs_r.emplace_back( &xs[i] );
  }
  xag_chain<true> chain_separate;
  encode( chain_separate, xag );
  chain_simulator<xag_chain<true>, kitty::dynamic_truth_table> sim_separate;
  sim_separate( chain_separate, xs_r );
  kitty::dynamic_truth_table tt4_separate( 4u ), tt5_separate( 4u ), tt6_separate( 4u );
  sim_separate.get_simulation_inline( tt4_separate, chain_separate, xs_r, 10u );
  sim_separate.get_simulation_inline( tt5_separate, chain_separate, xs_r, 12u );
  sim_separate.get_simulation_inline( tt6_separate, chain_separate, xs_r, 14u );
  CHECK( kitty::equal( xs[4], tt4_separate ) );
  CHECK( kitty::equal( xs[5], tt5_separate ) );
  CHECK( kitty::equal( xs[6], tt6_separate ) );

  xag_chain<false> chain_unified;
  encode( chain_unified, xag );
  chain_simulator<xag_chain<false>, kitty::dynamic_truth_table> sim_unified;
  sim_unified( chain_unified, xs_r );
  kitty::dynamic_truth_table tt4_unified( 4u ), tt5_unified( 4u ), tt6_unified( 4u );
  sim_unified.get_simulation_inline( tt4_unified, chain_unified, xs_r, 10u );
  sim_unified.get_simulation_inline( tt5_unified, chain_unified, xs_r, 12u );
  sim_unified.get_simulation_inline( tt6_unified, chain_unified, xs_r, 14u );
  CHECK( kitty::equal( xs[4], tt4_unified ) );
  CHECK( kitty::equal( xs[5], tt5_unified ) );
  CHECK( kitty::equal( xs[6], tt6_unified ) );
}

TEST_CASE( "simulation of mig_chain with static truth tables", "[chain_simulator]" )
{
  mig_network mig;
  auto const a = mig.create_pi();
  auto const b = mig.create_pi();
  auto const c = mig.create_pi();
  auto const d = mig.create_pi();
  auto const t0 = mig.create_and( a, b );
  auto const t1 = mig.create_and( c, d );
  auto const t2 = mig.create_or( t0, t1 );
  mig.create_po( t2 );

  std::vector<kitty::static_truth_table<4u>> xs;
  for ( auto i = 0u; i < 4u; ++i )
  {
    xs.emplace_back();
    kitty::create_nth_var( xs[i], i );
  }
  xs.emplace_back( xs[0] & xs[1] ); // 4
  xs.emplace_back( xs[2] & xs[3] ); // 5
  xs.emplace_back( xs[4] | xs[5] ); // 6

  std::vector<kitty::static_truth_table<4u> const*> xs_r;
  for ( auto i = 0u; i < 4u; ++i )
  {
    xs_r.emplace_back( &xs[i] );
  }
  mig_chain chain;
  encode( chain, mig );

  chain_simulator<mig_chain, kitty::static_truth_table<4u>> sim;
  sim( chain, xs_r );
  kitty::static_truth_table<4u> tt4, tt5, tt6;
  sim.get_simulation_inline( tt4, chain, xs_r, 10u );
  sim.get_simulation_inline( tt5, chain, xs_r, 12u );
  sim.get_simulation_inline( tt6, chain, xs_r, 14u );
  CHECK( kitty::equal( xs[4], tt4 ) );
  CHECK( kitty::equal( xs[5], tt5 ) );
  CHECK( kitty::equal( xs[6], tt6 ) );
}

TEST_CASE( "simulation of mig_chain with dynamic truth tables", "[chain_simulator]" )
{
  mig_network mig;
  auto const a = mig.create_pi();
  auto const b = mig.create_pi();
  auto const c = mig.create_pi();
  auto const d = mig.create_pi();
  auto const t0 = mig.create_and( a, b );
  auto const t1 = mig.create_and( c, d );
  auto const t2 = mig.create_or( t0, t1 );
  mig.create_po( t2 );

  std::vector<kitty::dynamic_truth_table> xs;
  for ( auto i = 0u; i < 4u; ++i )
  {
    xs.emplace_back( 4u );
    kitty::create_nth_var( xs[i], i );
  }
  xs.emplace_back( xs[0] & xs[1] ); // 4
  xs.emplace_back( xs[2] & xs[3] ); // 5
  xs.emplace_back( xs[4] | xs[5] ); // 6

  std::vector<kitty::dynamic_truth_table const*> xs_r;
  for ( auto i = 0u; i < 4u; ++i )
  {
    xs_r.emplace_back( &xs[i] );
  }
  mig_chain chain;
  encode( chain, mig );
  chain_simulator<mig_chain, kitty::dynamic_truth_table> sim;
  sim( chain, xs_r );
  kitty::dynamic_truth_table tt4( 4u ), tt5( 4u ), tt6( 4u );
  sim.get_simulation_inline( tt4, chain, xs_r, 10u );
  sim.get_simulation_inline( tt5, chain, xs_r, 12u );
  sim.get_simulation_inline( tt6, chain, xs_r, 14u );
  CHECK( kitty::equal( xs[4], tt4 ) );
  CHECK( kitty::equal( xs[5], tt5 ) );
  CHECK( kitty::equal( xs[6], tt6 ) );
}

std::string const test_library = "GATE   zero    0 O=CONST0;\n"                                                     // 0
                                 "GATE   one     0 O=CONST1;\n"                                                     // 1
                                 "GATE   inv1    1 O=!a;                      PIN * INV 1 999 0.9 0.3 0.9 0.3\n"    // 2
                                 "GATE   inv2    2 O=!a;                      PIN * INV 2 999 1.0 0.1 1.0 0.1\n"    // 3
                                 "GATE   buf     2 O=a;                       PIN * NONINV 1 999 1.0 0.0 1.0 0.0\n" // 4
                                 "GATE   nand    2 O=!(a*b);                  PIN * INV 1 999 1.0 0.2 1.0 0.2\n"    // 5
                                 "GATE   maj3    8 O=(a*b)+(a*c)+(b*c);       PIN * INV 1 999 3.0 0.4 3.0 0.4\n";   // 6

TEST_CASE( "simulation of bound_chain with static truth tables", "[chain_simulator]" )
{

  std::vector<gate> gates;
  std::istringstream in_genlib( test_library );
  auto result = lorina::read_genlib( in_genlib, genlib_reader( gates ) );
  CHECK( result == lorina::return_code::success );

  bound_chain<mad_hatter::networks::design_type_t::CELL_BASED> chain;
  chain.add_inputs( 4u );
  auto const a = chain.pi_at( 0 );
  auto const b = chain.pi_at( 1 );
  auto const c = chain.pi_at( 2 );
  auto const d = chain.pi_at( 3 );
  auto const lit2 = chain.add_gate( { a, b, c }, 6 );
  auto const lit3 = chain.add_gate( { lit2, d }, 5 );
  auto const lit4 = chain.add_gate( { lit3 }, 2 );
  chain.add_output( lit2 );
  chain.add_output( lit4 );

  std::vector<kitty::static_truth_table<4u>> xs;
  for ( auto i = 0u; i < 4u; ++i )
  {
    xs.emplace_back();
    kitty::create_nth_var( xs[i], i );
  }
  xs.emplace_back( ( xs[1] & xs[2] ) | ( xs[0] & xs[1] ) | ( xs[0] & xs[2] ) ); // 6
  xs.emplace_back( ~( xs.back() & xs[3] ) );                                    // 7
  xs.emplace_back( ~xs.back() );                                                // 8

  std::vector<kitty::static_truth_table<4u> const*> xs_r;
  for ( auto i = 0u; i < 4u; ++i )
  {
    xs_r.emplace_back( &xs[i] );
  }

  augmented_library<mad_hatter::networks::design_type_t::CELL_BASED> lib( gates );
  chain_simulator<bound_chain<mad_hatter::networks::design_type_t::CELL_BASED>, kitty::static_truth_table<4u>> sim( lib );
  sim( chain, xs_r );
  for ( auto i = 0u; i < xs.size(); ++i )
  {
    CHECK( kitty::equal( xs[i], sim.get_simulation( chain, xs_r, i ) ) );
  }
}
