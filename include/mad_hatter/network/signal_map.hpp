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
  \file signal_map.hpp
  \brief Map indexed by network signals

  \author Andrea Costamagna
*/

#pragma once

#include "../traits.hpp"

#include <cassert>
#include <memory>
#include <unordered_map>
#include <variant>
#include <vector>

#include <mockturtle/traits.hpp>

namespace mad_hatter
{

namespace network
{

/*! \brief Vector-based signal map with validity query
 *
 * This container is a variant of the `incomplete_node_map` tailored for cases
 * in which different signals pointing to the same node should store different
 * values. A crucial use case is when signals contain bitfields for specifying
 * the output in a multiple output gate.
 *
 * The implementation uses a vector as underlying data structure, so
 * that it benefits from fast access. It is supplemented with an
 * additional validity field such that it can be used like an
 * `unordered_signal_map`.
 *
 * **Required network functions:**
 * - `signal_size`
 * - `signal_to_index`
 *
 */
template<class T, class Ntk>
class incomplete_signal_map
{
public:
  using signal = typename Ntk::signal;
  using container_type = std::vector<T>;

  explicit incomplete_signal_map( Ntk const& ntk, T const& init_value = T() )
      : ntk( &ntk ),
        data( ntk.signal_size(), init_value )
  {
    static_assert( mockturtle::is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( mad_hatter::traits::has_signal_size_v<Ntk>, "Ntk does not implement signal_size" );
    static_assert( mad_hatter::traits::has_signal_to_index_v<Ntk>, "Ntk does not implement signal_to_index" );
  }

  auto size() const { return data.size(); }

  T& operator[]( signal const& f )
  {
    auto idx = ntk->signal_to_index( f );
    if ( idx >= data.size() )
      data.resize( idx + 1 );
    return data[idx];
  }

  T const& operator[]( signal const& f ) const
  {
    auto idx = ntk->signal_to_index( f );
    assert( idx < data.size() );
    return data[idx];
  }

  void reset( T const& init_value = T() )
  {
    data.assign( ntk->signal_size(), init_value );
  }

  void resize()
  {
    if ( ntk->signal_size() > data.size() )
      data.resize( ntk->signal_size() );
  }

private:
  Ntk const* ntk;
  std::vector<T> data;
};

} /* namespace network */

} /* namespace mad_hatter */