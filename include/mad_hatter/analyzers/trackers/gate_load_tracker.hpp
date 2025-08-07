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
 * \file load_capacitance_tracker.hpp
 * \brief Compute arrival time information of a network and updates upon change
 *
 * \author Andrea Costamagna
 */

#pragma once

#include "../../network/signal_map.hpp"
#include <limits>

namespace mad_hatter
{

namespace analyzers
{

namespace trackers
{

/*! \brief Engine to evaluate the arrival times of a network.
 *
 * This engine computes the arrival times of a network and keeps them up-to-date.
 * During construction it is possible to specify a vector of arrival times. If
 * not provided, the engine assumes 0 arrival time at all the PIs. At construction,
 * the arrival times are propagated in the network.
 *
 * The engine is equipped with a transitive fanout (TFO) manager to handle the
 * timing updates. Two events trigger the update:
 * - Node addition: The arrival time of the node is computed from the fanins
 * - Node modification: The arrival time of the TFO of the affected nodes is updated.
 *
 * \tparam Ntk the network type to be analyzed.
 *
 * \verbatim embed:rst

   Example

   .. code-block:: c++

      bound_network ntk( gates );
      auto const a = ntk.create_pi();
      auto const f1 = ntk.create_node( { a }, 0 );
      gate_load_tracker arrival( ntk );
      auto const f2 = ntk.create_node( { f1 }, 0 );
   \endverbatim
 */
template<class Ntk>
class gate_load_tracker
{
public:
  using node_index_t = typename Ntk::node;
  using signal_t = typename Ntk::signal;

public:
  gate_load_tracker( Ntk& ntk )
      : ntk_( ntk ),
        loads_( ntk )
  {
    init();
  }

  void init()
  {
    /* check if the network implements the needed functionalities */
    static_assert( mockturtle::is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( mockturtle::has_incr_trav_id_v<Ntk>, "Ntk does not implement the incr_trav_id method" );
    static_assert( mockturtle::has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
    static_assert( mad_hatter::traits::has_foreach_output_v<Ntk>, "Ntk does not implement the foreach_output method" );
    static_assert( mockturtle::has_foreach_fanout_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
    static_assert( mockturtle::has_foreach_pi_v<Ntk>, "Ntk does not implement the foreach_pi method" );
    static_assert( mockturtle::has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_pi method" );
    static_assert( mockturtle::has_set_value_v<Ntk>, "Ntk does not implement the set_visited method" );
    static_assert( mockturtle::has_value_v<Ntk>, "Ntk does not implement the set_visited method" );
    static_assert( mockturtle::has_set_visited_v<Ntk>, "Ntk does not implement the set_visited method" );
    static_assert( mockturtle::has_visited_v<Ntk>, "Ntk does not implement the set_visited method" );
    static_assert( mockturtle::has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
    static_assert( mockturtle::has_size_v<Ntk>, "Ntk does not implement the size method" );

    compute_gate_load();

    add_event_ = ntk_.events().register_add_event( [&]( const node_index_t& n ) {
      loads_.resize();
      ntk_.foreach_output( n, [&]( auto f ) {
        loads_[f] = 0;
        ntk_.foreach_fanin( n, [&]( auto const& fi, auto ii ) {
          loads_[fi] += ntk_.get_input_load( f, ii );
        } );
      } );
    } );

    delete_event_ = ntk_.events().register_delete_event( [&]( const node_index_t& n ) {
      ntk_.foreach_output( n, [&]( auto f ) {
        loads_[f] = 0;
        ntk_.foreach_fanin( n, [&]( auto const& fi, auto ii ) {
          loads_[fi] -= ntk_.get_input_load( f, ii );
        } );
      } );
    } );

    modified_event_ = ntk_.events().register_modified_event( [&]( const auto& n, auto old_children ) {
      ntk_.foreach_output( n, [&]( auto const& f ) {
        ntk_.foreach_fanin( n, [&]( auto const& fi, auto ii ) {
          if ( fi != old_children[ii] )
          {
            loads_[fi] += ntk_.get_input_load( f, ii );
            loads_[old_children[ii]] -= ntk_.get_input_load( f, ii );
          }
        } );
      } );
    } );
  }

  ~gate_load_tracker()
  {
    if ( add_event_ )
    {
      ntk_.events().release_add_event( add_event_ );
    }

    if ( delete_event_ )
    {
      ntk_.events().release_delete_event( delete_event_ );
    }

    if ( modified_event_ )
    {
      ntk_.events().release_modified_event( modified_event_ );
    }
  }

#pragma region Interface methods
public:
  [[nodiscard]] double get_load( signal_t const f ) const
  {
    return loads_[f];
  }
#pragma endregion

#pragma region Implementation details
private:
  bool is_marked_ready( node_index_t const& n )
  {
    return ntk_.value( n ) == ntk_.trav_id();
  };

  void make_ready( node_index_t const& n )
  {
    ntk_.set_value( n, ntk_.trav_id() );
  };

  void compute_gate_load()
  {
    ntk_.incr_trav_id();
    loads_.reset( 0 );

    ntk_.foreach_po( [&]( auto f ) {
      compute_gate_load_tfi( f );
    } );

    ntk_.foreach_po( [&]( auto f ) {
      loads_[f] = std::max( loads_[f], 1.0 );
    } );
  }

  /*! \brief Compute the arrival times of the nodes in the TFI of a signal's node*/
  void compute_gate_load_tfi( signal_t const& f )
  {
    node_index_t n = ntk_.get_node( f );
    if ( is_marked_ready( n ) || ntk_.is_pi( n ) )
    {
      return;
    }

    ntk_.foreach_fanin( n, [&]( auto fi, auto ii ) {
      compute_gate_load_tfi( fi );
      loads_[fi] += ntk_.get_input_load( f, ii );
    } );

    make_ready( n );
  }

#pragma endregion

private:
  Ntk& ntk_;
  network::incomplete_signal_map<double, Ntk> loads_;
  /* events */
  std::shared_ptr<typename mockturtle::network_events<Ntk>::add_event_type> add_event_;
  std::shared_ptr<typename mockturtle::network_events<Ntk>::delete_event_type> delete_event_;
  std::shared_ptr<typename mockturtle::network_events<Ntk>::modified_event_type> modified_event_;
};
} // namespace trackers

} // namespace analyzers

} // namespace mad_hatter