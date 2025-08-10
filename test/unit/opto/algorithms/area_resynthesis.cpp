#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <kitty/kitty.hpp>
#include <kitty/static_truth_table.hpp>

#include <mad_hatter/network/network.hpp>
#include <mad_hatter/opto/algorithms/resynthesize.hpp>
#include <mockturtle/io/genlib_reader.hpp>
#include <mockturtle/utils/tech_library.hpp>
#include <mockturtle/views/depth_view.hpp>

std::string const test_library = "GATE   and2    1.0 O=a*b;                 PIN * INV 1   999 1.0 0.0 1.0 0.0\n"
                                 "GATE   or2     1.0 O=a+b;                 PIN * INV 1   999 1.0 0.0 1.0 0.0\n"
                                 "GATE   xor2    0.5 O=a^b;                 PIN * INV 1   999 1.0 0.0 1.0 0.0\n"
                                 "GATE   or3     1.0 O=a+b+c;               PIN * INV 1   999 1.0 0.0 1.0 0.0\n"
                                 "GATE   and3    1.0 O=((a*b)*c);           PIN * INV 1   999 1.0 0.0 1.0 0.0\n"
                                 "GATE   maj3    1.0 O=(a*b)+(b*c)+(a*c);   PIN * INV 1   999 1.0 0.0 1.0 0.0\n"
                                 "GATE   fa      1.0 C=a*b+a*c+b*c;         PIN * INV 1   999 1.0 0.0 1.0 0.0\n"
                                 "GATE   fa      1.0 S=a^b^c;               PIN * INV 1   999 1.0 0.0 1.0 0.0\n"
                                 "GATE   nand2   1.0 O=!(a*b);              PIN * INV 1   999 1.0 0.0 1.0 0.0\n"
                                 "GATE   inv1    1.0 O=!a;                  PIN * INV 1   999 1.0 0.0 1.0 0.0";

struct custom_area_rewire_params : mad_hatter::opto::algorithms::default_resynthesis_params<8u>
{
  bool try_rewire = true;
  bool try_struct = false;
  bool try_window = false;
  bool try_simula = false;
  bool dynamic_database = false;
  int32_t odc_levels = 0;
  static constexpr uint32_t max_cuts_size = 6u;
};

TEST_CASE( "Area resynthesis via rewiring - single-output gate without don't cares", "[area_resynthesis]" )
{
  using Ntk = mad_hatter::network::bound_network<mad_hatter::network::design_type_t::CELL_BASED, 2>;
  using signal = typename Ntk::signal;
  std::vector<mockturtle::gate> gates;

  std::istringstream in( test_library );
  auto result = lorina::read_genlib( in, mockturtle::genlib_reader( gates ) );
  CHECK( result == lorina::return_code::success );

  mad_hatter::libraries::augmented_library<mad_hatter::network::design_type_t::CELL_BASED> lib( gates );

  static constexpr uint32_t MaxNumVars = 6u;
  using Db = mad_hatter::databases::mapped_database<Ntk, MaxNumVars>;
  Db db( lib );

  Ntk ntk( gates );
  auto const a = ntk.create_pi();
  auto const b = ntk.create_pi();
  auto const c = ntk.create_pi();
  auto const d = ntk.create_pi();
  auto const e = ntk.create_pi();
  auto const f1 = ntk.create_node( { c, d, e }, 4u );
  auto const f2 = ntk.create_node( { a, b }, 0u );
  auto const f3 = ntk.create_node( { c, d }, 0u );
  auto const f4 = ntk.create_node( { e, f3 }, 0u );
  auto const f5 = ntk.create_node( { f2, f4 }, 0u );

  ntk.create_po( f1 );
  ntk.create_po( f5 );

  using DNtk = mockturtle::depth_view<Ntk>;
  DNtk dntk( ntk );
  custom_area_rewire_params ps;
  mad_hatter::opto::algorithms::area_resynthesize<DNtk, Db, custom_area_rewire_params>( dntk, db, ps );
  CHECK( ntk.area() == 3 );
}

TEST_CASE( "Area resynthesis via rewiring - single-output gate with don't cares", "[area_resynthesis]" )
{
  using Ntk = mad_hatter::network::bound_network<mad_hatter::network::design_type_t::CELL_BASED, 2>;
  using signal = typename Ntk::signal;
  std::vector<mockturtle::gate> gates;

  std::istringstream in( test_library );
  auto result = lorina::read_genlib( in, mockturtle::genlib_reader( gates ) );
  CHECK( result == lorina::return_code::success );

  mad_hatter::libraries::augmented_library<mad_hatter::network::design_type_t::CELL_BASED> lib( gates );

  static constexpr uint32_t MaxNumVars = 6u;
  using Db = mad_hatter::databases::mapped_database<Ntk, MaxNumVars>;
  Db db( lib );

  Ntk ntk( gates );
  auto const a = ntk.create_pi();
  auto const b = ntk.create_pi();
  auto const c = ntk.create_pi();
  auto const d = ntk.create_pi();
  auto const f1 = ntk.create_node( { a, b }, 0u );
  auto const f2 = ntk.create_node( { c, d }, 1u );
  auto const f3 = ntk.create_node( { c, d }, 0u );
  auto const f4 = ntk.create_node( { c, d }, 2u );
  auto const f5 = ntk.create_node( { f1, f2 }, 0u );
  auto const f6 = ntk.create_node( { f3, f5 }, 1u );
  auto const f7 = ntk.create_node( { f3, f4 }, 0u );

  ntk.create_po( f6 );
  ntk.create_po( f7 );

  using DNtk = mockturtle::depth_view<Ntk>;
  mad_hatter::windowing::window_manager_stats st;
  DNtk dntk( ntk );
  custom_area_rewire_params ps;
  ps.window_manager_ps.odc_levels = 3;
  mad_hatter::opto::algorithms::area_resynthesize<DNtk, Db, custom_area_rewire_params>( dntk, db, ps );
  CHECK( ntk.area() == 5.5 );
}

TEST_CASE( "Area resynthesis via rewiring - multiple-output gate without don't cares", "[area_resynthesis]" )
{
  using Ntk = mad_hatter::network::bound_network<mad_hatter::network::design_type_t::CELL_BASED, 2>;
  using signal = typename Ntk::signal;
  std::vector<mockturtle::gate> gates;

  std::istringstream in( test_library );
  auto result = lorina::read_genlib( in, mockturtle::genlib_reader( gates ) );
  CHECK( result == lorina::return_code::success );

  mad_hatter::libraries::augmented_library<mad_hatter::network::design_type_t::CELL_BASED> lib( gates );

  static constexpr uint32_t MaxNumVars = 6u;
  using Db = mad_hatter::databases::mapped_database<Ntk, MaxNumVars>;
  Db db( lib );

  Ntk ntk( gates );
  auto const a = ntk.create_pi();
  auto const b = ntk.create_pi();
  auto const c = ntk.create_pi();
  auto const d = ntk.create_pi();
  auto const e = ntk.create_pi();
  auto const f1 = ntk.create_node( { c, d, e }, 4u );
  auto const f2 = ntk.create_node( { a, b }, 0u );
  auto const f3 = ntk.create_node( { c, d }, 0u );
  auto const f4 = ntk.create_node( { e, f3 }, 0u );
  auto const f5 = ntk.create_node( { c, f2, f4 }, { 6u, 7u } );

  ntk.create_po( f1 );
  ntk.create_po( { f5.index, 0 } );
  ntk.create_po( { f5.index, 1 } );

  using DNtk = mockturtle::depth_view<Ntk>;
  mad_hatter::windowing::window_manager_stats st;
  DNtk dntk( ntk );
  custom_area_rewire_params ps;
  mad_hatter::opto::algorithms::area_resynthesize<DNtk, Db, custom_area_rewire_params>( dntk, db, ps );
  CHECK( ntk.area() == 3 );
}

TEST_CASE( "Area resynthesis via rewiring - multiple-output gate with don't cares", "[area_resynthesis]" )
{
  using Ntk = mad_hatter::network::bound_network<mad_hatter::network::design_type_t::CELL_BASED, 2>;
  using signal = typename Ntk::signal;
  std::vector<mockturtle::gate> gates;

  std::istringstream in( test_library );
  auto result = lorina::read_genlib( in, mockturtle::genlib_reader( gates ) );
  CHECK( result == lorina::return_code::success );

  mad_hatter::libraries::augmented_library<mad_hatter::network::design_type_t::CELL_BASED> lib( gates );

  static constexpr uint32_t MaxNumVars = 6u;
  using Db = mad_hatter::databases::mapped_database<Ntk, MaxNumVars>;
  Db db( lib );

  Ntk ntk( gates );
  auto const a = ntk.create_pi();
  auto const b = ntk.create_pi();
  auto const c = ntk.create_pi();
  auto const d = ntk.create_pi();
  auto const e = ntk.create_pi();
  auto const f1 = ntk.create_node( { a, b }, 0u );
  auto const f2 = ntk.create_node( { c, d }, 1u );
  auto const f3 = ntk.create_node( { c, d }, 0u );
  auto const f4 = ntk.create_node( { c, d }, 2u );
  auto const f5 = ntk.create_node( { e, f1, f2 }, { 6u, 7u } );
  auto const f6 = ntk.create_node( { { f5.index, 0 }, e }, 0u );
  auto const f7 = ntk.create_node( { { f5.index, 1 }, f4 }, 0u );
  auto const f8 = ntk.create_node( { f6, f7 }, 0u );
  auto const f9 = ntk.create_node( { f3, f8 }, 1u );

  ntk.create_po( f9 );

  using DNtk = mockturtle::depth_view<Ntk>;
  mad_hatter::windowing::window_manager_stats st;
  DNtk dntk( ntk );
  custom_area_rewire_params ps;
  mad_hatter::opto::algorithms::area_resynthesize<DNtk, Db, custom_area_rewire_params>( dntk, db, ps );
  CHECK( ntk.area() == 7.5 );
}

struct custom_area_struct_params1 : mad_hatter::opto::algorithms::default_resynthesis_params<8u>
{
  bool try_rewire = false;
  bool try_struct = true;
  bool try_window = false;
  bool try_simula = false;
  bool dynamic_database = false;
  int32_t odc_levels = 0;
  static constexpr uint32_t max_cuts_size = 6u;
};

struct custom_area_struct_params2 : mad_hatter::opto::algorithms::default_resynthesis_params<8u>
{
  bool try_rewire = false;
  bool try_struct = true;
  bool try_window = false;
  bool try_simula = false;
  bool dynamic_database = false;
  int32_t odc_levels = 2;
  static constexpr uint32_t max_cuts_size = 6u;
};

TEST_CASE( "Area resynthesis via cut rewriting - single-output gate without don't cares", "[area_resynthesis]" )
{
  using Ntk = mad_hatter::network::bound_network<mad_hatter::network::design_type_t::CELL_BASED, 2>;
  using signal = typename Ntk::signal;
  std::vector<mockturtle::gate> gates;

  std::istringstream in( test_library );
  auto result = lorina::read_genlib( in, mockturtle::genlib_reader( gates ) );
  CHECK( result == lorina::return_code::success );

  mad_hatter::libraries::augmented_library<mad_hatter::network::design_type_t::CELL_BASED> lib( gates );

  static constexpr uint32_t MaxNumVars = 6u;
  using Db = mad_hatter::databases::mapped_database<Ntk, MaxNumVars>;
  Db db( lib );

  mad_hatter::evaluation::chains::bound_chain<mad_hatter::network::design_type_t::CELL_BASED> list;
  list.add_inputs( MaxNumVars );
  list.add_output( list.add_gate( { list.add_gate( { 0, 1 }, 8 ), 2 }, 1 ) );
  CHECK( db.add( list ) );

  Ntk ntk( gates );
  auto const a = ntk.create_pi();
  auto const b = ntk.create_pi();
  auto const c = ntk.create_pi();
  auto const f1 = ntk.create_node( { c, a }, 0u );
  auto const f2 = ntk.create_node( { b }, 9u );
  auto const f3 = ntk.create_node( { f1, f2 }, 0u );
  auto const f4 = ntk.create_node( { f3 }, 9u );

  ntk.create_po( f4 );

  using DNtk = mockturtle::depth_view<Ntk>;
  mad_hatter::windowing::window_manager_stats st;
  DNtk dntk( ntk );
  custom_area_struct_params1 ps;
  mad_hatter::opto::algorithms::area_resynthesize<DNtk, Db, custom_area_struct_params1>( dntk, db, ps );
  CHECK( ntk.area() == 2.0 );
}

struct custom_area_struct_params3 : mad_hatter::opto::algorithms::default_resynthesis_params<6u>
{
  bool try_rewire = false;
  bool try_struct = true;
  bool try_window = false;
  bool try_simula = false;
  bool dynamic_database = false;
  static constexpr uint32_t max_cuts_size = 3u;
};

TEST_CASE( "Area resynthesis via cut rewriting - single-output gate with don't cares", "[area_resynthesis]" )
{
  using Ntk = mad_hatter::network::bound_network<mad_hatter::network::design_type_t::CELL_BASED, 2>;
  using signal = typename Ntk::signal;
  std::vector<mockturtle::gate> gates;

  std::istringstream in( test_library );
  auto result = lorina::read_genlib( in, mockturtle::genlib_reader( gates ) );
  CHECK( result == lorina::return_code::success );

  mad_hatter::libraries::augmented_library<mad_hatter::network::design_type_t::CELL_BASED> lib( gates );

  static constexpr uint32_t MaxNumVars = 3u;
  using Db = mad_hatter::databases::mapped_database<Ntk, MaxNumVars>;
  Db db( lib );

  mad_hatter::evaluation::chains::bound_chain<mad_hatter::network::design_type_t::CELL_BASED> list1, list2;
  list1.add_inputs( MaxNumVars );
  list2.add_inputs( MaxNumVars );
  list1.add_output( list1.add_gate( { list1.add_gate( { 0, 1 }, 1 ), 2 }, 0 ) );
  list2.add_output( list2.add_gate( { list2.add_gate( { 0, 1 }, 2 ), 2 }, 0 ) );
  CHECK( db.add( list1 ) );
  CHECK( db.add( list2 ) );

  Ntk ntk( gates );
  auto const c = ntk.create_pi();
  auto const b = ntk.create_pi();
  auto const a = ntk.create_pi();
  auto const f1 = ntk.create_node( { a, b }, 0u );
  auto const f2 = ntk.create_node( { a, b }, 1u );
  auto const f3 = ntk.create_node( { f2, c }, 0u );
  auto const f4 = ntk.create_node( { f3, f1 }, 1u );

  ntk.create_po( f4 );

  using DNtk = mockturtle::depth_view<Ntk>;
  mad_hatter::windowing::window_manager_stats st;
  DNtk dntk( ntk );
  custom_area_struct_params3 ps;
  ps.window_manager_ps.odc_levels = 3;
  mad_hatter::opto::algorithms::area_resynthesize<DNtk, Db, custom_area_struct_params3>( dntk, db, ps );
  CHECK( ntk.area() == 3.5 );
}

struct custom_area_window_params1 : mad_hatter::opto::algorithms::default_resynthesis_params<6u>
{
  bool try_rewire = false;
  bool try_struct = false;
  bool try_window = true;
  bool try_simula = false;
  bool dynamic_database = false;
  static constexpr uint32_t max_cuts_size = 6u;
};

TEST_CASE( "Area resynthesis via window rewriting - single-output gate without don't cares", "[area_resynthesis]" )
{
  using Ntk = mad_hatter::network::bound_network<mad_hatter::network::design_type_t::CELL_BASED, 2>;
  using signal = typename Ntk::signal;
  std::vector<mockturtle::gate> gates;

  std::istringstream in( test_library );
  auto result = lorina::read_genlib( in, mockturtle::genlib_reader( gates ) );
  CHECK( result == lorina::return_code::success );

  mad_hatter::libraries::augmented_library<mad_hatter::network::design_type_t::CELL_BASED> lib( gates );

  static constexpr uint32_t MaxNumVars = 6u;
  using Db = mad_hatter::databases::mapped_database<Ntk, MaxNumVars>;
  Db db( lib );

  mad_hatter::evaluation::chains::bound_chain<mad_hatter::network::design_type_t::CELL_BASED> list;
  list.add_inputs( MaxNumVars );
  list.add_output( list.add_gate( { list.add_gate( { 0, 2 }, 2 ), 1 }, 2 ) );
  CHECK( db.add( list ) );

  Ntk ntk( gates );
  auto const a = ntk.create_pi();
  auto const b = ntk.create_pi();
  auto const c = ntk.create_pi();
  auto const f1 = ntk.create_node( { a, b }, 0u );
  auto const f2 = ntk.create_node( { a, c }, 0u );
  auto const f3 = ntk.create_node( { b, c }, 0u );
  auto const f4 = ntk.create_node( { f1, f2 }, 1u );
  auto const f5 = ntk.create_node( { f4, f3 }, 1u );

  ntk.create_po( f1 );
  ntk.create_po( f2 );
  ntk.create_po( f3 );
  ntk.create_po( f5 );

  using DNtk = mockturtle::depth_view<Ntk>;
  mad_hatter::windowing::window_manager_stats st;
  DNtk dntk( ntk );
  custom_area_window_params1 ps;
  mad_hatter::opto::algorithms::area_resynthesize<DNtk, Db, custom_area_window_params1>( dntk, db, ps );
  CHECK( ntk.area() == 4.0 );
}

TEST_CASE( "Area resynthesis via window rewriting - single-output gate with don't cares", "[area_resynthesis]" )
{
  using Ntk = mad_hatter::network::bound_network<mad_hatter::network::design_type_t::CELL_BASED, 2>;
  using signal = typename Ntk::signal;
  std::vector<mockturtle::gate> gates;

  std::istringstream in( test_library );
  auto result = lorina::read_genlib( in, mockturtle::genlib_reader( gates ) );
  CHECK( result == lorina::return_code::success );

  mad_hatter::libraries::augmented_library<mad_hatter::network::design_type_t::CELL_BASED> lib( gates );

  static constexpr uint32_t MaxNumVars = 3u;
  using Db = mad_hatter::databases::mapped_database<Ntk, MaxNumVars>;
  Db db( lib );

  mad_hatter::evaluation::chains::bound_chain<mad_hatter::network::design_type_t::CELL_BASED> list;
  list.add_inputs( MaxNumVars );
  list.add_output( list.add_gate( { list.add_gate( { 0, 1 }, 2 ), 2 }, 1 ) );
  CHECK( db.add( list ) );

  Ntk ntk( gates );
  auto const a = ntk.create_pi();
  auto const b = ntk.create_pi();
  auto const c = ntk.create_pi();
  auto const d = ntk.create_pi();
  auto const f1 = ntk.create_node( { a, b }, 0u );
  auto const f2 = ntk.create_node( { c, d }, 0u );
  auto const f3 = ntk.create_node( { b }, 9u );
  auto const f4 = ntk.create_node( { a, f3 }, 8u );
  auto const f5 = ntk.create_node( { f1, b }, 2u );
  auto const f6 = ntk.create_node( { f2, f5 }, 1u );
  auto const f7 = ntk.create_node( { f4, f6 }, 0u );

  ntk.create_po( f2 );
  ntk.create_po( f7 );

  using DNtk = mockturtle::depth_view<Ntk>;
  mad_hatter::windowing::window_manager_stats st;
  DNtk dntk( ntk );
  custom_area_window_params1 ps;
  ps.window_manager_ps.odc_levels = 3;
  mad_hatter::opto::algorithms::area_resynthesize<DNtk, Db, custom_area_window_params1>( dntk, db, ps );
  CHECK( ntk.area() == 5.5 );
}