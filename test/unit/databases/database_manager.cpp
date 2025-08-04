#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <mad_hatter/databases/database_manager.hpp>
#include <mad_hatter/evaluation/evaluation.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/xag.hpp>

using namespace mockturtle;

template<typename Ntk, typename Chain>
void test_npn_lookup()
{
  static constexpr uint32_t num_vars = 4u;
  using TT = kitty::static_truth_table<num_vars>;
  TT onset;
  mad_hatter::databases::database_manager<Ntk> mng;
  std::array<TT, num_vars> xs;
  std::vector<TT const*> xs_ptrs;
  for ( auto i = 0u; i < num_vars; ++i )
  {
    kitty::create_nth_var( xs[i], i );
    xs_ptrs.push_back( &xs[i] );
  }

  mad_hatter::evaluation::chain_simulator<Chain, TT> sim_chain;
  do
  {
    /* define the functionality */
    kitty::next_inplace( onset );
    kitty::ternary_truth_table<TT> tt( onset );

    /* boolean matching */
    auto info = mng.lookup_npn( tt );
    /* verify that at least a match is found */
    CHECK( info );
    Ntk ntk;
    std::vector<typename Ntk::signal> pis;
    for ( auto i = 0u; i < num_vars; ++i )
    {
      pis.push_back( ntk.create_pi() );
    }
    /* consider all the sub-networks matching the functionality */
    info->foreach_entry( [&]( auto f ) {
      /* insert the sub-network into a network and simulate it */
      auto o = mng.insert( *info, ntk, f, pis.begin(), pis.end() );
      auto no = ntk.get_node( o );

      default_simulator<TT> sim;
      auto tts = simulate_nodes<TT, Ntk>( ntk, sim );
      auto res = ntk.is_complemented( o ) ? ~tts[no] : tts[no];
      CHECK( kitty::equal( res, onset ) );

      /* insert the sub-network into a chain and simulate it */
      {
        Chain chain( num_vars );
        std::vector<uint32_t> pis_chain{ 2, 4, 6, 8 };
        auto lit_out = mng.insert( *info, chain, f, pis_chain.begin(), pis_chain.end() );
        sim_chain( chain, xs_ptrs );
        auto [p_res_chain, is_compl] = sim_chain.get_simulation( chain, xs_ptrs, lit_out );
        TT res_chain = *p_res_chain;
        if ( is_compl )
        {
          res_chain = ~res_chain;
        }
        CHECK( kitty::equal( res_chain, onset ) );
      }
    } );
  } while ( !kitty::is_const0( onset ) );
}

TEST_CASE( "database for aig_network", "[database_manager]" )
{
  test_npn_lookup<mockturtle::aig_network, mad_hatter::evaluation::chains::xag_chain<true>>();
}

TEST_CASE( "database for xag_network", "[database_manager]" )
{
  test_npn_lookup<mockturtle::xag_network, mad_hatter::evaluation::chains::xag_chain<true>>();
}

TEST_CASE( "database for mig_network", "[database_manager]" )
{
  test_npn_lookup<mockturtle::mig_network, mad_hatter::evaluation::chains::mig_chain>();
}