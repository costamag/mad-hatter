/* mad-hatter: C++ logic network library
 * Copyright (C) 2025  EPFL
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
  \file muxig_chain.hpp
  \brief List of indices to represent small Multiplexer Inverter Graphs (MUXIG)

  \author Andrea Costamagna
  \author Heinz Riener
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include <fmt/format.h>
#include <mockturtle/traits.hpp>

#include <vector>

namespace rinox
{

namespace evaluation
{

namespace chains
{

/*! \brief Index list for mux-inverter graphs.
 *
 * Small network consisting of mux gates and inverters
 * represented as a list of literals.
 *
 * Example: The following index list creates the output function
 * `<<x1 ? x2 : x3> ? x2 : x4>` with 4 inputs, 1 output, and 2 gates:
 * `{4 | 1 << 8 | 2 << 16, 2, 4, 6, 4, 8, 10, 12}`
 */
struct muxig_chain
{
public:
  using element_type = uint32_t;

public:
  explicit muxig_chain( uint32_t num_pis = 0 )
      : values( { num_pis } )
  {
  }

  explicit muxig_chain( std::vector<element_type> const& values )
      : values( std::begin( values ), std::end( values ) )
  {}

  std::vector<element_type> raw() const
  {
    return values;
  }

  uint64_t size() const
  {
    return values.size();
  }

  uint64_t num_gates() const
  {
    return ( values.at( 0 ) >> 16 );
  }

  uint64_t num_pis() const
  {
    return values.at( 0 ) & 0xff;
  }

  uint64_t num_pos() const
  {
    return ( values.at( 0 ) >> 8 ) & 0xff;
  }

  template<typename Fn>
  void foreach_gate( Fn&& fn ) const
  {
    assert( ( values.size() - 1u - num_pos() ) % 3 == 0 );
    for ( uint64_t i = 1u; i < values.size() - num_pos(); i += 3 )
    {
      fn( values.at( i ), values.at( i + 1 ), values.at( i + 2 ) );
    }
  }

  template<typename Fn>
  void foreach_po( Fn&& fn ) const
  {
    for ( uint64_t i = values.size() - num_pos(); i < values.size(); ++i )
    {
      fn( values.at( i ) );
    }
  }

  void clear()
  {
    values.clear();
    values.emplace_back( 0 );
  }

  void add_inputs( uint32_t n = 1u )
  {
    assert( num_pis() + n <= 0xff );
    values.at( 0u ) += n;
  }

  element_type add_mux( element_type lit0, element_type lit1, element_type lit2 )
  {
    assert( num_gates() + 1u <= 0xffff );
    values.at( 0u ) = ( ( num_gates() + 1 ) << 16 ) | ( values.at( 0 ) & 0xffff );
    values.push_back( lit0 );
    values.push_back( lit1 );
    values.push_back( lit2 );
    return ( num_gates() + num_pis() ) << 1;
  }

  void add_output( element_type lit )
  {
    assert( num_pos() + 1 <= 0xff );
    values.at( 0u ) = ( num_pos() + 1 ) << 8 | ( values.at( 0u ) & 0xffff00ff );
    values.push_back( lit );
  }

private:
  std::vector<element_type> values;
};

/*! \brief Inserts a muxig_chain into an existing network
 *
 * **Required network functions:**
 * - `get_constant`
 * - `create_ite`
 *
 * \param ntk A logic network
 * \param begin Begin iterator of signal inputs
 * \param end End iterator of signal inputs
 * \param indices An index list
 * \param fn Callback function
 */
template<bool useSignal = true, typename Ntk, typename BeginIter, typename EndIter, typename Fn>
void insert( Ntk& ntk, BeginIter begin, EndIter end, muxig_chain const& indices, Fn&& fn )
{
  static_assert( mockturtle::is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( mockturtle::has_create_ite_v<Ntk>, "Ntk does not implement the create_maj method" );
  static_assert( mockturtle::has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );

  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

  if constexpr ( useSignal )
  {
    static_assert( std::is_same_v<std::decay_t<typename std::iterator_traits<BeginIter>::value_type>, signal>, "BeginIter value_type must be Ntk signal type" );
    static_assert( std::is_same_v<std::decay_t<typename std::iterator_traits<EndIter>::value_type>, signal>, "EndIter value_type must be Ntk signal type" );
  }
  else
  {
    static_assert( std::is_same_v<std::decay_t<typename std::iterator_traits<BeginIter>::value_type>, node>, "BeginIter value_type must be Ntk node type" );
    static_assert( std::is_same_v<std::decay_t<typename std::iterator_traits<EndIter>::value_type>, node>, "EndIter value_type must be Ntk node type" );
  }

  assert( uint64_t( std::distance( begin, end ) ) == indices.num_pis() );

  std::vector<signal> signals;
  signals.emplace_back( ntk.get_constant( false ) );
  for ( auto it = begin; it != end; ++it )
  {
    if constexpr ( useSignal )
    {
      signals.push_back( *it );
    }
    else
    {
      signals.emplace_back( ntk.make_signal( *it ) );
    }
  }

  indices.foreach_gate( [&]( uint32_t lit0, uint32_t lit1, uint32_t lit2 ) {
    signal const s0 = ( lit0 % 2 ) ? !signals.at( lit0 >> 1 ) : signals.at( lit0 >> 1 );
    signal const s1 = ( lit1 % 2 ) ? !signals.at( lit1 >> 1 ) : signals.at( lit1 >> 1 );
    signal const s2 = ( lit2 % 2 ) ? !signals.at( lit2 >> 1 ) : signals.at( lit2 >> 1 );
    signals.push_back( ntk.create_ite( s0, s1, s2 ) );
  } );

  indices.foreach_po( [&]( uint32_t lit ) {
    uint32_t const i = lit >> 1;
    fn( ( lit % 2 ) ? !signals.at( i ) : signals.at( i ) );
  } );
}

/*! \brief Converts an mig_chain to a string
 *
 * \param indices An index list
 * \return A string representation of the index list
 */
inline std::string to_chain_string( muxig_chain const& indices )
{
  auto s = fmt::format( "{{{} pis | {} pos | {} gates", indices.num_pis(), indices.num_pos(), indices.num_gates() );

  indices.foreach_gate( [&]( uint32_t lit0, uint32_t lit1, uint32_t lit2 ) {
    s += fmt::format( ", ({} ? {} : {})", lit0, lit1, lit2 );
  } );

  indices.foreach_po( [&]( uint32_t lit ) {
    s += fmt::format( ", {}", lit );
  } );

  s += "}";

  return s;
}

} // namespace chains

} // namespace evaluation

} // namespace rinox
