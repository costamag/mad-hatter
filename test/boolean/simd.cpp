// SPDX-License-Identifier: MIT
// Tests for mad_hatter boolean/simd_operations.hpp

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/operations.hpp>
#include <kitty/static_truth_table.hpp>
#include <mad_hatter/boolean/simd.hpp>

namespace nspace = mad_hatter::boolean;

TEST_CASE( "SIMD set_zero large tables", "[simd]" )
{
  using TTS = kitty::static_truth_table<12u>;
  using TTD = kitty::dynamic_truth_table;

  TTS tts;
  nspace::test_avx2_advantage( tts, 12u );

  nspace::set_zero( tts );
  REQUIRE( tts == ( tts ^ tts ) ); // all zero

  TTD ttd( 12u );
  nspace::test_avx2_advantage( ttd, 12u );

  nspace::set_zero( ttd );
  REQUIRE( ttd == ( ttd ^ ttd ) ); // all zero
}

TEST_CASE( "SIMD nspace::set_ones large tables", "[simd]" )
{
  using TTS = kitty::static_truth_table<12u>;
  using TTD = kitty::dynamic_truth_table;

  TTS tts;
  nspace::test_avx2_advantage( tts, 12u );

  nspace::set_ones( tts );
  REQUIRE( tts == ( tts ^ ~tts ) ); // all ones

  TTD ttd( 12u );
  nspace::test_avx2_advantage( ttd, 12u );

  nspace::set_ones( ttd );
  REQUIRE( ttd == ( ttd ^ ~ttd ) ); // all ones
}

TEST_CASE( "SIMD nspace::binary AND large", "[simd]" )
{
  using TTS = kitty::static_truth_table<12u>;
  using TTD = kitty::dynamic_truth_table;

  TTS tts;
  TTS tta, ttb;
  kitty::create_random( tta, 0 );
  kitty::create_random( ttb, 1 );

  auto ref = tta & ttb;
  auto simd_res = nspace::binary_and( tta, ttb );
  REQUIRE( simd_res == ref );

  TTD ttd( 12u ), ttda( 12u ), ttdb( 12u );
  kitty::create_random( ttda, 2 );
  kitty::create_random( ttdb, 3 );

  auto refd = ttda & ttdb;
  auto simd_resd = nspace::binary_and( ttda, ttdb );
  REQUIRE( simd_resd == refd );
}

TEST_CASE( "SIMD nspace::binary XOR large", "[simd]" )
{
  using TTS = kitty::static_truth_table<12u>;
  using TTD = kitty::dynamic_truth_table;

  TTS tta, ttb;
  kitty::create_random( tta, 0 );
  kitty::create_random( ttb, 1 );

  auto ref = tta ^ ttb;
  auto simd_res = nspace::binary_xor( tta, ttb );
  REQUIRE( simd_res == ref );

  TTD ttda( 12u ), ttdb( 12u );
  kitty::create_random( ttda, 2 );
  kitty::create_random( ttdb, 3 );

  auto refd = ttda ^ ttdb;
  auto simd_resd = nspace::binary_xor( ttda, ttdb );
  REQUIRE( simd_resd == refd );
}

TEST_CASE( "SIMD nspace::binary OR large", "[simd]" )
{
  using TTS = kitty::static_truth_table<12u>;
  using TTD = kitty::dynamic_truth_table;

  TTS tta, ttb;
  kitty::create_random( tta, 0 );
  kitty::create_random( ttb, 1 );

  auto ref = tta | ttb;
  auto simd_res = nspace::binary_or( tta, ttb );
  REQUIRE( simd_res == ref );

  TTD ttda( 12u ), ttdb( 12u );
  kitty::create_random( ttda, 2 );
  kitty::create_random( ttdb, 3 );

  auto refd = ttda | ttdb;
  auto simd_resd = nspace::binary_or( ttda, ttdb );
  REQUIRE( simd_resd == refd );
}

TEST_CASE( "SIMD nspace::binary LT large", "[simd]" )
{
  using TTS = kitty::static_truth_table<12u>;
  using TTD = kitty::dynamic_truth_table;

  TTS tta, ttb;
  kitty::create_random( tta, 0 );
  kitty::create_random( ttb, 1 );

  auto ref = ( ~tta ) & ttb;
  auto simd_res = nspace::binary_lt( tta, ttb );
  REQUIRE( simd_res == ref );

  TTD ttda( 12u ), ttdb( 12u );
  kitty::create_random( ttda, 2 );
  kitty::create_random( ttdb, 3 );

  auto refd = ( ~ttda ) & ttdb;
  auto simd_resd = nspace::binary_lt( ttda, ttdb );
  REQUIRE( simd_resd == refd );
}
