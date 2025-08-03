/* mad-hatter: C++ logic network library
 * Copyright (C) 2025 EPFL
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*!
  \file symmetry.hpp
  \brief Implements utilities to handle functional symmetries

  \author Andrea Costamagna
*/

#pragma once

#include <kitty/kitty.hpp>

namespace mad_hatter
{

namespace boolean
{

struct symmetries_t
{
  uint64_t data = 0u;

  symmetries_t() = default;

  template<typename TT>
  symmetries_t( TT const& tt )
  {
    data = 0u;
    assert( tt.num_vars() <= 8 );

    for ( auto i = 0u; i < tt.num_vars(); ++i )
    {
      auto const tt1 = kitty::cofactor1( tt, i );
      auto const tt0 = kitty::cofactor0( tt, i );
      if ( kitty::equal( tt0, tt1 ) )
        continue;
      for ( auto j = i + 1; j < tt.num_vars(); ++j )
      {
        auto const tt1 = kitty::cofactor1( tt, j );
        auto const tt0 = kitty::cofactor0( tt, j );
        if ( kitty::equal( tt0, tt1 ) )
          continue;

        auto const tt01 = kitty::cofactor0( tt1, i );
        auto const tt10 = kitty::cofactor1( tt0, i );
        if ( kitty::equal( tt01, tt10 ) )
        {
          set( i, j );
        }
      }
    }
  }

  constexpr void set( uint8_t i, uint8_t j ) noexcept
  {
    uint64_t const mask = ( 1lu << j ) | ( 1lu << i );
    data |= ( mask << ( 8 * i ) );
    data |= ( mask << ( 8 * j ) );
  }

  constexpr bool symmetric( uint8_t i, uint8_t j ) const noexcept
  {
    return ( ( ( data >> ( 8 * i ) ) >> j ) & ( ( data >> ( 8 * j ) ) >> i ) & 0x1 ) > 0;
  }

  constexpr bool has_symmetries( uint8_t i ) const noexcept
  {
    return ( ( data >> ( 8 * i ) ) & 0xFF ) > 0;
  }
};

template<typename CompFn, typename... Vecs>
void sort_symmetric( symmetries_t const& symm,
                     CompFn&& fn,
                     Vecs&... vecs )
{
  // All vectors must have the same size
  const size_t n = std::get<0>( std::tuple{ vecs... } ).size();
  assert( ( ( vecs.size() == n ) && ... ) && "[e] all vectors must have the same size" );

  // Use the first vector as the driver
  auto& driver = std::get<0>( std::forward_as_tuple( vecs... ) );

  std::vector<uint8_t> inputs( n );
  std::iota( inputs.begin(), inputs.end(), 0 );

  for ( uint8_t i = 0; i < n; ++i )
  {
    if ( symm.has_symmetries( i ) )
    {
      uint8_t k = i;
      int j = i - 1;
      auto value = driver[inputs[i]];
      bool swapped = true;

      while ( swapped && ( j >= 0 ) )
      {
        if ( symm.symmetric( inputs[k], inputs[j] ) )
        {
          if ( fn( value, driver[j] ) )
          {
            // Apply the swap to all vectors
            ( std::swap( vecs[k], vecs[j] ), ... );
            std::swap( inputs[k], inputs[j] );
            k = j;
            swapped = true;
          }
          else
          {
            swapped = false;
          }
        }
        j--;
      }
    }
  }
}

} // namespace boolean

} // namespace mad_hatter