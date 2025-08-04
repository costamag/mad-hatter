#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <sstream>
#include <string>

#include <mad_hatter/io/verilog/verilog.hpp>
#include <mad_hatter/network/network.hpp>
#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/networks/buffered.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/muxig.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/networks/xmg.hpp>

#include <kitty/kitty.hpp>
#include <lorina/verilog.hpp>

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

TEST_CASE( "Read structural verilog to mapped network", "[verilog_reader]" )
{

  using bound_network = mad_hatter::network::bound_network<mad_hatter::network::design_type_t::CELL_BASED, 2>;
  std::vector<mockturtle::gate> gates;

  std::istringstream in_lib( test_library );
  auto result_lib = lorina::read_genlib( in_lib, genlib_reader( gates ) );
  CHECK( result_lib == lorina::return_code::success );

  std::string file{
      "module top( x0 , x1 , x2 , y0 , y1 , y2, y3 );\n"
      "  input x0 , x1, x2 ;\n"
      "  output y0 , y1 , y2, y3 ;\n"
      "  wire n4 , n5 , n6 ;\n"
      "  inv1 g0( .a (x0), .O (n4) );\n"
      "  fa   g1( .a (n4), .b (x1), .c (x2), .C (n5), .S (n6) );\n"
      "  inv1 g2( .a (n4), .O (y0) );\n"
      "  xor2 g3( .a (n6), .b (x2), .O (y1) );\n"
      "  buf g4( .a (n5), .O (y2) );\n"
      "  buf g5( .a (n6), .O (y3) );\n"
      "endmodule\n" };

  std::istringstream in_ntk( file );

  bound_network ntk( gates );
  const auto result_ntk = mad_hatter::io::verilog::read_verilog( in_ntk, mad_hatter::io::verilog::verilog_reader( ntk ) );

  /* structural checks */
  CHECK( result_ntk == lorina::return_code::success );
  CHECK( ntk.num_pis() == 3 );
  CHECK( ntk.num_pos() == 4 );
  CHECK( ntk.size() == 11 ); // 2 constants, 2 PIs, 3 buffers, 1 inverter, 3 crossings, 3 gates

  CHECK( ntk.num_gates() == 6 );

  std::ostringstream out;
  mad_hatter::io::verilog::write_verilog( ntk, out );

  std::string expected = "module top( x0 , x1 , x2 , y0 , y1 , y2 , y3 );\n"
                         "  input x0 , x1 , x2 ;\n"
                         "  output y0 , y1 , y2 , y3 ;\n"
                         "  wire n5 , n6_0 , n6_1 ;\n"
                         "  inv1  g0( .a (x0), .O (n5) );\n"
                         "  inv1  g1( .a (n5), .O (y0) );\n"
                         "  fa    g2( .a (n5), .b (x1), .c (x2), .C (n6_0), .S (n6_1) );\n"
                         "  xor2  g3( .a (n6_1), .b (x2), .O (y1) );\n"
                         "  buf   g4( .a (n6_0), .O (y2) );\n"
                         "  buf   g5( .a (n6_1), .O (y3) );\n"
                         "endmodule\n";
  CHECK( out.str() == expected );
}

TEST_CASE( "Read structural verilog to mapped network  with the inputs permutated", "[verilog_reader]" )
{

  using bound_network = mad_hatter::network::bound_network<mad_hatter::network::design_type_t::CELL_BASED, 2>;
  std::vector<mockturtle::gate> gates;

  std::istringstream in_lib( test_library );
  auto result_lib = lorina::read_genlib( in_lib, genlib_reader( gates ) );
  CHECK( result_lib == lorina::return_code::success );

  std::string file{
      "module top( x0 , x1 , x2 , y0 , y1 , y2, y3 );\n"
      "  input x0 , x1, x2 ;\n"
      "  output y0 , y1 , y2, y3 ;\n"
      "  wire n4 , n5 , n6 ;\n"
      "  inv1 g0( .a (x0), .O (n4) );\n"
      "  fa   g1( .b (x1), .C (n5), .a (n4), .c (x2), .S (n6) );\n"
      "  inv1 g2( .a (n4), .O (y0) );\n"
      "  xor2 g3( .a (n6), .O (y1), .b (x2) );\n"
      "  buf g4( .a (n5), .O (y2) );\n"
      "  buf g5( .O (y3), .a (n6) );\n"
      "endmodule\n" };

  std::istringstream in_ntk( file );

  bound_network ntk( gates );
  const auto result_ntk = mad_hatter::io::verilog::read_verilog( in_ntk, mad_hatter::io::verilog::verilog_reader( ntk ) );

  /* structural checks */
  CHECK( result_ntk == lorina::return_code::success );
  CHECK( ntk.num_pis() == 3 );
  CHECK( ntk.num_pos() == 4 );
  CHECK( ntk.size() == 11 ); // 2 constants, 2 PIs, 3 buffers, 1 inverter, 3 crossings, 3 gates

  CHECK( ntk.num_gates() == 6 );

  std::ostringstream out;
  mad_hatter::io::verilog::write_verilog( ntk, out );

  std::string expected = "module top( x0 , x1 , x2 , y0 , y1 , y2 , y3 );\n"
                         "  input x0 , x1 , x2 ;\n"
                         "  output y0 , y1 , y2 , y3 ;\n"
                         "  wire n5 , n6_0 , n6_1 ;\n"
                         "  inv1  g0( .a (x0), .O (n5) );\n"
                         "  inv1  g1( .a (n5), .O (y0) );\n"
                         "  fa    g2( .a (n5), .b (x1), .c (x2), .C (n6_0), .S (n6_1) );\n"
                         "  xor2  g3( .a (n6_1), .b (x2), .O (y1) );\n"
                         "  buf   g4( .a (n6_0), .O (y2) );\n"
                         "  buf   g5( .a (n6_1), .O (y3) );\n"
                         "endmodule\n";
  CHECK( out.str() == expected );
}

TEST_CASE( "Read structural verilog to mapped network in the format used by abc", "[verilog_reader]" )
{

  using bound_network = mad_hatter::network::bound_network<mad_hatter::network::design_type_t::CELL_BASED, 2>;
  std::vector<mockturtle::gate> gates;

  std::istringstream in_lib( test_library );
  auto result_lib = lorina::read_genlib( in_lib, genlib_reader( gates ) );
  CHECK( result_lib == lorina::return_code::success );

  std::string file = R"(module top( \a[0] , \a[1] , \b[0] , \f[0] , \f[1] , \f[2], \f[3] );
    input \a[0] , \a[1] , \b[0] ;
    output \f[0] , \f[1] , \f[2], \f[3] ;
    wire n4 , n5 , n6 ;
    inv1 g0( .a (\a[0]), .O (n4) );
    fa   g1( .b (\a[1]), .C (n5), .a (n4), .c (\b[0]), .S (n6) );
    inv1 g2( .a (n4), .O (\f[0]) );
    xor2 g3( .a (n6), .O (\f[1]), .b (\b[0]) );
    buf g4( .a (n5), .O (\f[2]) );
    buf g5( .O (\f[3]), .a (n6) );
    endmodule
    )";

  std::istringstream in_ntk( file );

  bound_network ntk( gates );
  const auto result_ntk = mad_hatter::io::verilog::read_verilog( in_ntk, mad_hatter::io::verilog::verilog_reader( ntk ) );

  /* structural checks */
  CHECK( result_ntk == lorina::return_code::success );
  CHECK( ntk.num_pis() == 3 );
  CHECK( ntk.num_pos() == 4 );
  CHECK( ntk.size() == 11 ); // 2 constants, 2 PIs, 3 buffers, 1 inverter, 3 crossings, 3 gates

  CHECK( ntk.num_gates() == 6 );

  std::ostringstream out;
  mad_hatter::io::verilog::write_verilog( ntk, out );

  std::string expected = "module top( x0 , x1 , x2 , y0 , y1 , y2 , y3 );\n"
                         "  input x0 , x1 , x2 ;\n"
                         "  output y0 , y1 , y2 , y3 ;\n"
                         "  wire n5 , n6_0 , n6_1 ;\n"
                         "  inv1  g0( .a (x0), .O (n5) );\n"
                         "  inv1  g1( .a (n5), .O (y0) );\n"
                         "  fa    g2( .a (n5), .b (x1), .c (x2), .C (n6_0), .S (n6_1) );\n"
                         "  xor2  g3( .a (n6_1), .b (x2), .O (y1) );\n"
                         "  buf   g4( .a (n6_0), .O (y2) );\n"
                         "  buf   g5( .a (n6_1), .O (y3) );\n"
                         "endmodule\n";
  CHECK( out.str() == expected );
}