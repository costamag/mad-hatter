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
  \file network_utils.hpp
  \brief Main header for index list representations of small networks.

  \author Andrea Costamagna
*/

#pragma once

#include <vector>

namespace mad_hatter
{

namespace networks
{

/*! \brief Count the number of nodes in the transitive fanin
 *
 * **Required network functions for the network Ntk:**
 * - `is_pi`
 * - `foreach_fanin`
 * - `get_node`
 * - `incr_trav_id`
 * - `set_visited`
 * - `trav_id`
 *
 * \param ntk The logic network
 * \param f The root signal of the logic cone
 */

template<typename Ntk>
size_t count_nodes( Ntk& ntk, typename Ntk::signal const& f )
{
  static_assert( mockturtle::is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( mockturtle::has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( mockturtle::has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( mockturtle::has_incr_trav_id_v<Ntk>, "Ntk does not implement the incr_trav_id method" );
  static_assert( mockturtle::has_set_visited_v<Ntk>, "Ntk does not implement the set_visited method" );
  static_assert( mockturtle::has_trav_id_v<Ntk>, "Ntk does not implement the trav_id method" );
  ntk.incr_trav_id();

  std::function<size_t( typename Ntk::signal const& )> count_nodes_r = [&]( typename Ntk::signal const& s ) {
    typename Ntk::node n = ntk.get_node( s );
    if ( ntk.is_pi( n ) || ( ntk.visited( n ) == ntk.trav_id() ) )
    {
      return static_cast<size_t>( 0u );
    }
    ntk.set_visited( n, ntk.trav_id() );
    size_t num_nodes = 1u;
    ntk.foreach_fanin( n, [&]( auto fi ) {
      num_nodes += count_nodes_r( fi );
    } );
    return num_nodes;
  };

  return count_nodes_r( f );
}

} // namespace network

} // namespace mad_hatter
