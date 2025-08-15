#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <kitty/kitty.hpp>

#include <rinox/synthesis/lut_decomposer.hpp>
#include <rinox/synthesis/synthesis.hpp>

using namespace rinox::synthesis;
using namespace rinox::evaluation;
using namespace rinox::evaluation::chains;
using namespace mockturtle;

template<uint32_t NumVars, uint32_t CutSize, typename Func, bool ExacSuppMin>
void test_lut_dec( Func const& func, std::vector<double> const& times, std::vector<std::vector<uint8_t>> const& supps, std::vector<Func> const& funcs )
{
  lut_decomposer<CutSize, NumVars, ExacSuppMin> decomposer;
  CHECK( decomposer.run( func, times ) );

  int j = 0;
  bool res = decomposer.foreach_spec( [&]( auto const specs, auto i ) {
    {
      CHECK( specs[i].inputs == supps[j] );
      CHECK( kitty::equal( specs[i].sim, funcs[j++] ) );
      return true;
    }
  } );
  CHECK( res );
}

TEST_CASE( "LUT decomposer for parity 4 3- shannon", "[lut_synthesis]" )
{
  static constexpr uint32_t num_vars = 3u;
  static constexpr uint32_t cut_size = 4u;

  using ctt_s = kitty::static_truth_table<num_vars>;
  using itt_s = kitty::ternary_truth_table<ctt_s>;
  using ctt_l = kitty::static_truth_table<cut_size>;
  using itt_l = kitty::ternary_truth_table<ctt_l>;

  ctt_l ctt;
  kitty::create_parity( ctt );
  itt_l func( ctt );
  std::vector<double> times{ 0, 0, 0, 3 };

  std::vector<std::vector<uint8_t>> supps;
  supps = { { 0, 1, 2 }, { 0, 1, 2 }, { 3, 4, 5 } };
  std::vector<ctt_l> ctts_l( 3u );
  kitty::create_from_binary_string( ctts_l[0], "1001011010010110" );
  kitty::create_from_binary_string( ctts_l[1], "0110100101101001" );
  kitty::create_from_binary_string( ctts_l[2], "0110100110010110" );
  std::vector<itt_l> funcs;
  funcs.emplace_back( ctts_l[0] );
  funcs.emplace_back( ctts_l[1] );
  funcs.emplace_back( ctts_l[2] );
  test_lut_dec<num_vars, cut_size, itt_l, /* exact support minimization */ true>( func, times, supps, funcs );
  test_lut_dec<num_vars, cut_size, itt_l, /* exact support minimization */ false>( func, times, supps, funcs );
}

TEST_CASE( "LUT decomposer for parity 5 3- shannon", "[lut_synthesis]" )
{
  static constexpr uint32_t num_vars = 3u;
  static constexpr uint32_t cut_size = 5u;

  using ctt_s = kitty::static_truth_table<num_vars>;
  using itt_s = kitty::ternary_truth_table<ctt_s>;
  using ctt_l = kitty::static_truth_table<cut_size>;
  using itt_l = kitty::ternary_truth_table<ctt_l>;

  ctt_l ctt;
  kitty::create_parity( ctt );
  itt_l func( ctt );
  std::vector<double> times{ 0, 0, 0, 0, 3 };

  std::vector<std::vector<uint8_t>> supps;
  supps = { { 3, 1, 2 }, { 3, 1, 2 }, { 0, 5, 6 }, { 3, 1, 2 }, { 3, 1, 2 }, { 0, 8, 9 }, { 4, 7, 10 } };
  std::vector<ctt_l> ctts_l( 7u );
  kitty::create_from_binary_string( ctts_l[0], "11000011001111001100001100111100" );
  kitty::create_from_binary_string( ctts_l[1], "00111100110000110011110011000011" );
  kitty::create_from_binary_string( ctts_l[2], "01101001100101100110100110010110" );
  kitty::create_from_binary_string( ctts_l[3], "00111100110000110011110011000011" );
  kitty::create_from_binary_string( ctts_l[4], "11000011001111001100001100111100" );
  kitty::create_from_binary_string( ctts_l[5], "10010110011010011001011001101001" );
  kitty::create_from_binary_string( ctts_l[6], "10010110011010010110100110010110" );
  std::vector<itt_l> funcs;
  funcs.emplace_back( ctts_l[0] );
  funcs.emplace_back( ctts_l[1] );
  funcs.emplace_back( ctts_l[2] );
  funcs.emplace_back( ctts_l[3] );
  funcs.emplace_back( ctts_l[4] );
  funcs.emplace_back( ctts_l[5] );
  funcs.emplace_back( ctts_l[6] );
  test_lut_dec<num_vars, cut_size, itt_l, /* exact support minimization */ true>( func, times, supps, funcs );
  test_lut_dec<num_vars, cut_size, itt_l, /* exact support minimization */ false>( func, times, supps, funcs );
}
