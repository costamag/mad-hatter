#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <kitty/kitty.hpp>
#include <kitty/static_truth_table.hpp>

#include <mad_hatter/network/network.hpp>
#include <mad_hatter/windowing/window_manager.hpp>

#include <mockturtle/io/genlib_reader.hpp>
#include <mockturtle/utils/tech_library.hpp>
#include <mockturtle/views/depth_view.hpp>

std::string const test_library = "GATE   inv1    1.0 O=!a ;         PIN * INV 1   999 1.0 0.0 1.0 0.0\n"
                                 "GATE   and2    1.0 O=a*b;         PIN * INV 1   999 1.0 0.0 1.0 0.0\n"
                                 "GATE   xor2    1.0 O=a^b;         PIN * INV 1   999 3.0 0.0 3.0 0.0";

struct window_manager_params1 : mad_hatter::windowing::default_window_manager_params
{
  static constexpr uint32_t max_num_leaves = 3u;
};

struct window_manager_params2 : mad_hatter::windowing::default_window_manager_params
{
  static constexpr uint32_t max_num_leaves = 8u;
};

TEST_CASE( "Window construction with reconvergent structure", "[window_manager]" )
{

  using Ntk = mad_hatter::network::bound_network<mad_hatter::network::design_type_t::CELL_BASED, 2>;
  std::vector<mockturtle::gate> gates;

  std::istringstream in( test_library );
  auto result = lorina::read_genlib( in, mockturtle::genlib_reader( gates ) );
  CHECK( result == lorina::return_code::success );

  Ntk ntk( gates );

  using signal = typename Ntk::signal;
  std::vector<signal> fs;
  /* Database construction from network */
  signal const a = ntk.create_pi();
  signal const b = ntk.create_pi();
  signal const c = ntk.create_pi();

  fs.push_back( ntk.create_node( { a, b }, 1 ) );           // 0
  fs.push_back( ntk.create_node( { a }, 0 ) );              // 1
  fs.push_back( ntk.create_node( { a, b }, 2 ) );           // 2
  fs.push_back( ntk.create_node( { b, c }, 2 ) );           // 3
  fs.push_back( ntk.create_node( { b, c }, 1 ) );           // 4
  fs.push_back( ntk.create_node( { c }, 0 ) );              // 5
  fs.push_back( ntk.create_node( { b, fs[5] }, 1 ) );       // 6
  fs.push_back( ntk.create_node( { fs[0], fs[1] }, 1 ) );   // 7
  fs.push_back( ntk.create_node( { fs[2], fs[1] }, 2 ) );   // 8
  fs.push_back( ntk.create_node( { fs[2], fs[3] }, 2 ) );   // 9
  fs.push_back( ntk.create_node( { fs[4], fs[3] }, 2 ) );   // 10
  fs.push_back( ntk.create_node( { fs[8], fs[9] }, 2 ) );   // 11
  fs.push_back( ntk.create_node( { fs[9], fs[10] }, 2 ) );  // 12
  fs.push_back( ntk.create_node( { fs[11], fs[12] }, 2 ) ); // 13
  fs.push_back( ntk.create_node( { fs[13], fs[7] }, 2 ) );  // 14
  fs.push_back( ntk.create_node( { fs[13], fs[6] }, 2 ) );  // 15
  fs.push_back( ntk.create_node( { fs[14], fs[1] }, 2 ) );  // 16
  fs.push_back( ntk.create_node( { fs[14], fs[15] }, 2 ) ); // 17
  fs.push_back( ntk.create_node( { fs[5], fs[15] }, 2 ) );  // 18
  fs.push_back( ntk.create_node( { fs[16] }, 0 ) );         // 19
  fs.push_back( ntk.create_node( { fs[17] }, 0 ) );         // 20

  ntk.create_po( fs[19] );
  ntk.create_po( fs[20] );
  ntk.create_po( fs[18] );

  using DNtk = mockturtle::depth_view<Ntk>;
  mad_hatter::windowing::window_manager_stats st;
  DNtk dntk( ntk );

  window_manager_params1 ps1;
  ps1.odc_levels = 3u;

  mad_hatter::windowing::window_manager<DNtk, window_manager_params1> window( dntk, ps1, st );

  CHECK( window.run( dntk.get_node( fs[13] ) ) );
  auto const mffc = window.get_mffc();
  CHECK( mffc == std::vector<typename Ntk::node_index_t>{ 16, 17, 18 } );
  auto const tfos = window.get_tfos();
  CHECK( tfos == std::vector<typename Ntk::node_index_t>{} );
  auto const outputs = window.get_outputs();
  CHECK( outputs == std::vector<typename Ntk::signal_t>{ fs[13] } );
  auto const leaves = window.get_leaves();
  CHECK( leaves == std::vector<typename Ntk::signal_t>( { fs[8], fs[9], fs[10] } ) );

  window_manager_params2 ps2;
  ps2.odc_levels = 3u;

  mad_hatter::windowing::window_manager<DNtk, window_manager_params2> window2( dntk, ps2, st );

  CHECK( window2.run( dntk.get_node( fs[13] ) ) );
  auto const mffc2 = window2.get_mffc();
  CHECK( mffc2 == std::vector<typename Ntk::node_index_t>{ 7, 9, 8, 13, 14, 15, 16, 17, 18 } );
  auto const tfos2 = window2.get_tfos();
  CHECK( tfos2 == std::vector<typename Ntk::node_index_t>{ 19, 20, 21, 22 } );
  auto const outputs2 = window2.get_outputs();
  CHECK( outputs2 == std::vector<typename Ntk::signal_t>( { fs[18], fs[19], fs[20] } ) );
  auto const leaves2 = window2.get_leaves();
  CHECK( leaves2 == std::vector<typename Ntk::signal_t>( { a, b, c } ) );
}