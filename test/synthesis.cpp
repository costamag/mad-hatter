#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <kitty/kitty.hpp>

#include <mad_hatter/dependency/dependency.hpp>
#include <mad_hatter/evaluation/evaluation.hpp>
#include <mad_hatter/synthesis/synthesis.hpp>
#include <mad_hatter/synthesis/xaig_decompose.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/xag.hpp>

using namespace mad_hatter::synthesis;
using namespace mad_hatter::evaluation;
using namespace mad_hatter::evaluation::chains;
using namespace mockturtle;

/* remove Boolean matching with don't cares from testing */
bool constexpr xaig_decompose_tests_with_dcs = false;

#if xaig_decompose_tests_with_dcs
TEST_CASE( "XAIG synththesizer - constants", "[synthesis]" )
{
  xaig_decompose_stats st;
  constexpr uint32_t NumVars = 5u;
  using TT = kitty::static_truth_table<NumVars>;
  constexpr bool UseDCs = true;
  xaig_decompose<UseDCs> engine( st );
  TT onset, careset;
  std::vector<uint32_t> raw;
  large_xag_chain chain;
  kitty::create_from_hex_string( onset, "0000000A" );
  kitty::create_from_hex_string( careset, "FFFFFFF0" );
  kitty::ternary_truth_table<TT> const0( onset, careset );

  engine( const0 );
  chain = engine.get_chain();
  raw = { 5, 1, 0, 0 };
  CHECK( raw == chain.raw() );

  kitty::create_from_hex_string( onset, "FFFFFFFA" );
  kitty::ternary_truth_table<TT> const1( onset, careset );
  engine( const1 );
  chain = engine.get_chain();
  raw = { 5, 1, 0, 1 };
  CHECK( raw == chain.raw() );
}

TEST_CASE( "XAIG synththesizer - projections", "[synthesis]" )
{
  xaig_decompose_stats st;
  constexpr uint32_t NumVars = 7u;
  constexpr bool UseDCs = true;
  xaig_decompose<UseDCs> engine( st );
  using TT = kitty::static_truth_table<NumVars>;
  TT onset, careset;
  std::vector<uint32_t> raw;
  large_xag_chain chain;
  std::array<kitty::ternary_truth_table<TT>, NumVars> proj_fns;
  for ( auto i = 0u; i < NumVars; ++i )
  {
    kitty::create_nth_var( proj_fns[i], i );
  }

  kitty::create_from_hex_string( onset, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAFF" );
  kitty::create_from_hex_string( careset, "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00" );
  kitty::ternary_truth_table<TT> func0( onset, careset );
  engine( func0 );
  chain = engine.get_chain();
  raw = { 7, 1, 0, 2 };
  CHECK( raw == chain.raw() );

  kitty::create_from_hex_string( onset, "555555555555555555555555555555FF" );
  kitty::ternary_truth_table<TT> func1( onset, careset );
  engine( func1 );
  chain = engine.get_chain();
  raw = { 7, 1, 0, 3 };
  CHECK( raw == chain.raw() );
}
#endif

template<uint32_t NumVars>
void test_xag_n_input_functions()
{
  xaig_decompose_stats st;
  xaig_decompose engine( st );
  using TT = kitty::static_truth_table<NumVars>;
  kitty::static_truth_table<NumVars> onset;
  large_xag_chain chain;

  chain_simulator<xag_chain<true>, TT> sim;
  std::vector<TT> divisor_functions;
  TT tmp;
  for ( auto i = 0u; i < NumVars; ++i )
  {
    kitty::create_nth_var( tmp, i );
    divisor_functions.emplace_back( tmp );
  }
  std::vector<TT const*> xs_r;
  for ( auto i = 0u; i < NumVars; ++i )
  {
    xs_r.emplace_back( &divisor_functions[i] );
  }
  do
  {
    kitty::ternary_truth_table<TT> tt( onset );
    engine( tt );
    auto chain = engine.get_chain();

    sim( chain, xs_r );
    TT res;
    sim.get_simulation_inline( res, chain, xs_r, chain.po_at( 0 ) );
    CHECK( onset == res );

    kitty::next_inplace( onset );
  } while ( !kitty::is_const0( onset ) );
}

TEST_CASE( "XAIG synththesizer - 3 input functions", "[synthesis]" )
{
  test_xag_n_input_functions<3>();
}

template<uint32_t NumVars>
void test_xag_n_input_functions_random()
{
  xaig_decompose_stats st;
  xaig_decompose engine( st );
  using TT = kitty::static_truth_table<NumVars>;
  kitty::static_truth_table<NumVars> onset;
  large_xag_chain chain;

  chain_simulator<xag_chain<true>, TT> sim;
  std::vector<TT> divisor_functions;
  TT tmp;
  for ( auto i = 0u; i < NumVars; ++i )
  {
    kitty::create_nth_var( tmp, i );
    divisor_functions.emplace_back( tmp );
  }
  std::vector<TT const*> xs_r;
  for ( auto i = 0u; i < NumVars; ++i )
  {
    xs_r.emplace_back( &divisor_functions[i] );
  }
  int i = 1;
  do
  {
    kitty::ternary_truth_table<TT> tt( onset );
    engine( tt );
    auto chain = engine.get_chain();

    sim( chain, xs_r );
    TT res;
    sim.get_simulation_inline( res, chain, xs_r, chain.po_at( 0 ) );
    CHECK( onset == res );
    kitty::create_random( onset, 2 * i++ );
  } while ( i < 1000 );
}

TEST_CASE( "XAIG synththesizer - random 10 input functions", "[synthesis]" )
{
  test_xag_n_input_functions_random<10>();
}

TEST_CASE( "Termination condition for LUT decomposition", "[synthesis]" )
{
  static constexpr uint32_t MaxNumVars = 2u;
  static constexpr uint32_t MaxCutSize = 3u;
  using CSTT = kitty::static_truth_table<MaxCutSize>;
  using ISTT = kitty::ternary_truth_table<CSTT>;
  mad_hatter::synthesis::lut_decomposer<MaxCutSize, MaxNumVars> decomposer;

  CSTT care1, mask1;
  kitty::create_from_binary_string( care1, "10001100" );
  kitty::create_from_binary_string( mask1, "11111011" );
  ISTT func1( care1, mask1 );
  std::vector<double> times{ 0.0, 0.0, 0.0 };
  CHECK( decomposer.run( func1, times ) );
  decomposer.foreach_spec( [&]( auto const& sim_ptrs, auto const& spec ) {
    kitty::static_truth_table<2u> expected;
    kitty::create_from_binary_string( expected, "1000" );
    auto itt = mad_hatter::dependency::extract_function<kitty::static_truth_table<MaxCutSize>, MaxNumVars>( sim_ptrs, spec.sim._bits, spec.sim._care );
    CHECK( kitty::equal( expected, itt._bits ) );
    CHECK( kitty::is_const0( ~itt._care ) );
    return std::make_optional( *sim_ptrs[0] );
  } );
}