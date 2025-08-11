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

TEST_CASE( "Corner case 1 - assign constant", "[verilog_reader]" )
{

  using bound_network = mad_hatter::network::bound_network<mad_hatter::network::design_type_t::CELL_BASED, 2>;
  std::vector<mockturtle::gate> gates;

  std::istringstream in_lib( test_library );
  auto result_lib = lorina::read_genlib( in_lib, genlib_reader( gates ) );
  CHECK( result_lib == lorina::return_code::success );

  std::string file =
      R"(module top(a, y);
  input a;
  output y;
  wire _7_;

  assign _7_ = 1'h0;

  buf g0 (.a(_7_), .O(y));
  endmodule
  )";

  std::istringstream in_ntk( file );

  bound_network ntk( gates );
  const auto result_ntk = mad_hatter::io::verilog::read_verilog( in_ntk, mad_hatter::io::verilog::verilog_reader( ntk ) );

  /* structural checks */
  CHECK( result_ntk == lorina::return_code::success );
  CHECK( ntk.num_pis() == 1 );
  CHECK( ntk.num_pos() == 1 );
  CHECK( ntk.size() == 4 ); // 2 constants, 2 PIs, 3 buffers, 1 inverter, 3 crossings, 3 gates

  CHECK( ntk.num_gates() == 1 );

  std::ostringstream out;
  mad_hatter::io::verilog::write_verilog( ntk, out );

  std::string expected =
      "module top( x0 , y0 );\n"
      "  input x0 ;\n"
      "  output y0 ;\n"
      "  buf   g0( .a (0), .O (y0) );\n"
      "endmodule\n";

  CHECK( out.str() == expected );
}

TEST_CASE( "Corner case 2 - vector assignment with positive polarity", "[verilog_reader]" )
{

  using bound_network = mad_hatter::network::bound_network<mad_hatter::network::design_type_t::CELL_BASED, 2>;
  std::vector<mockturtle::gate> gates;

  std::istringstream in_lib( test_library );
  auto result_lib = lorina::read_genlib( in_lib, genlib_reader( gates ) );
  CHECK( result_lib == lorina::return_code::success );

  std::string file =
      R"(module top( \xyz_inst.a[0] , \xyz_inst.a[1] , \xyz_inst.a[2] , \xyz_inst.a[3] ,
            \xyz_inst.i0.a[0] , \xyz_inst.i0.a[1] , \xyz_inst.i0.a[2] , \xyz_inst.i0.a[3] );
    // Port directions
    input  \xyz_inst.a[0] , \xyz_inst.a[1] , \xyz_inst.a[2] , \xyz_inst.a[3] ;
    output \xyz_inst.i0.a[0] , \xyz_inst.i0.a[1] , \xyz_inst.i0.a[2] , \xyz_inst.i0.a[3] ;

    // Intermediate net for variety
    wire tmp;

    // Vector-to-vector assign with escaped IDs
    assign { \xyz_inst.i0.a[3] , \xyz_inst.i0.a[2] , \xyz_inst.i0.a[1] , tmp } =
           { \xyz_inst.a[3]    , \xyz_inst.a[2]    , \xyz_inst.a[1]    , \xyz_inst.a[0]    };

    // Some trivial logic to touch tmp
    buf g1 (.a(tmp), .O(\xyz_inst.i0.a[0]));

endmodule
  )";

  std::istringstream in_ntk( file );

  bound_network ntk( gates );
  const auto result_ntk = mad_hatter::io::verilog::read_verilog( in_ntk, mad_hatter::io::verilog::verilog_reader( ntk ) );

  /* structural checks */
  CHECK( result_ntk == lorina::return_code::success );
  CHECK( ntk.num_pis() == 4 );
  CHECK( ntk.num_pos() == 4 );
  CHECK( ntk.size() == 7 ); // 2 constants, 4 PIs, 2 buffers
  CHECK( ntk.num_gates() == 1 );

  std::ostringstream out;
  mad_hatter::io::verilog::write_verilog( ntk, out );

  std::string expected =
      "module top( x0 , x1 , x2 , x3 , y0 , y1 , y2 , y3 );\n"
      "  input x0 , x1 , x2 , x3 ;\n"
      "  output y0 , y1 , y2 , y3 ;\n"
      "  buf   g0( .a (x0), .O (y0) );\n"
      "endmodule\n";

  CHECK( out.str() == expected );
}

TEST_CASE( "Corner case 3 - vector assignment with explicit positive polarity", "[verilog_reader]" )
{

  using bound_network = mad_hatter::network::bound_network<mad_hatter::network::design_type_t::CELL_BASED, 2>;
  std::vector<mockturtle::gate> gates;

  std::istringstream in_lib( test_library );
  auto result_lib = lorina::read_genlib( in_lib, genlib_reader( gates ) );
  CHECK( result_lib == lorina::return_code::success );

  std::string file =
      R"(module top( \xyz_inst.f00[0] , \xyz_inst.f01[0] , \xyz_inst.f02[0] , \xyz_inst.f03[0] ,
            \xyz_inst.f04[0] , \xyz_inst.f05[0] , \xyz_inst.f06[0] , \xyz_inst.f07[0] ,
            \xyz_inst.out0[0], \xyz_inst.out0[1], \xyz_inst.out0[2], \xyz_inst.out0[3],
            \xyz_inst.out0[4], \xyz_inst.out0[5], \xyz_inst.out0[6], \xyz_inst.out0[7] );

    // Port directions
    input  \xyz_inst.f00[0] , \xyz_inst.f01[0] , \xyz_inst.f02[0] , \xyz_inst.f03[0] ,
           \xyz_inst.f04[0] , \xyz_inst.f05[0] , \xyz_inst.f06[0] , \xyz_inst.f07[0] ;
    output \xyz_inst.out0[0], \xyz_inst.out0[1], \xyz_inst.out0[2], \xyz_inst.out0[3],
           \xyz_inst.out0[4], \xyz_inst.out0[5], \xyz_inst.out0[6], \xyz_inst.out0[7] ;

    // Internal nets for a couple of small gates
    wire n1, n2;

    // Two trivial logic gates
    and2 g0 (.a(\xyz_inst.f00[0]), .b(\xyz_inst.f01[0]), .O(n1));
    xor2  g1 (.a(\xyz_inst.f02[0]), .b(n1),               .O(n2));

    // Weird continuous assignment: concatenations with an addition in between
    assign { \xyz_inst.out0[7], \xyz_inst.out0[6], \xyz_inst.out0[5], \xyz_inst.out0[4],
             \xyz_inst.out0[3], \xyz_inst.out0[2], \xyz_inst.out0[1], \xyz_inst.out0[0] }
         = + { \xyz_inst.f07[0], \xyz_inst.f06[0], \xyz_inst.f05[0], \xyz_inst.f04[0],
                \xyz_inst.f03[0], n2, \xyz_inst.f01[0], \xyz_inst.f00[0] };

endmodule
  )";

  std::istringstream in_ntk( file );

  bound_network ntk( gates );
  const auto result_ntk = mad_hatter::io::verilog::read_verilog( in_ntk, mad_hatter::io::verilog::verilog_reader( ntk ) );

  /* structural checks */
  CHECK( result_ntk == lorina::return_code::success );
  CHECK( ntk.num_pis() == 8 );
  CHECK( ntk.num_pos() == 8 );
  CHECK( ntk.size() == 12 ); // 2 constants, 4 PIs, 2 buffers
  CHECK( ntk.num_gates() == 2 );

  std::ostringstream out;
  mad_hatter::io::verilog::write_verilog( ntk, out );

  std::string expected =
      "module top( x0 , x1 , x2 , x3 , x4 , x5 , x6 , x7 , y0 , y1 , y2 , y3 , y4 , y5 , y6 , y7 );\n"
      "  input x0 , x1 , x2 , x3 , x4 , x5 , x6 , x7 ;\n"
      "  output y0 , y1 , y2 , y3 , y4 , y5 , y6 , y7 ;\n"
      "  wire n10 ;\n"
      "  and2  g0( .a (y0), .b (y1), .O (n10) );\n"
      "  xor2  g1( .a (x2), .b (n10), .O (y2) );\n"
      "endmodule\n";

  CHECK( out.str() == expected );
}

/*

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


*/