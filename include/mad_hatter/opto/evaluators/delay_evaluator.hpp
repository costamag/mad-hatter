/* mad_hatter: C++ logic network library
 * Copyright (C) 2018-2023  EPFL
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
  \file delay_evaluator.hpp
  \brief Analyzer of the delay

  \author Andrea Costamagna
*/

#pragma once

#include "../../databases/mapped_database.hpp"
#include "../../trackers/arrival_times_tracker.hpp"
#include "../../trackers/required_times_tracker.hpp"
#include "evaluators_utils.hpp"

namespace mad_hatter
{

namespace opto
{

namespace evaluators
{

template<class Ntk>
class delay_evaluator
{
public:
  using node_index_t = typename Ntk::node;
  using signal_t = typename Ntk::signal;
  using cost_t = double;
  static cost_t constexpr min_cost = std::numeric_limits<cost_t>::min();
  static cost_t constexpr max_cost = std::numeric_limits<cost_t>::max();
  static bool constexpr pass_window = false;
  static bool constexpr node_depend = false;
  static bool constexpr has_arrival = true;

  struct node_with_cost_t
  {
    node_index_t root;
    cost_t mffc_delay;
  };

public:
  delay_evaluator( Ntk& ntk, evaluator_params const& ps )
      : ntk_( ntk ),
        ps_( ps ),
        nodes_( ntk_.size() ),
        arrival_( ntk_ )
  {
  }

  double get_arrival( signal_t const& f ) const
  {
    return arrival_.get_time( f );
  }

  template<class List_t>
  cost_t evaluate( List_t const& list, std::vector<signal_t> const& leaves )
  {
    signal_t const f = insert( ntk_, leaves, list );
    node_index_t const n = ntk_.get_node( f );
    cost_t time = 0.0;
    foreach_output( n, [&]( auto const& f ) {
      time = std::max( time, arrival_.get_time( f ) );
    } );
    if ( ntk_.fanout_size( n ) == 0 )
      ntk_.take_out_node( n );
    return time;
  }

  cost_t evaluate_rewiring( node_index_t const& n, std::vector<signal_t> const& new_children, std::vector<signal_t> const& win_leaves )
  {
    auto const lib = ntk_.get_library();
    cost_t curr_cost = 0.0;
    foreach_output( n, [&]( auto const& f ) {
      curr_cost = std::max( curr_cost, arrival_.get_time( f ) );
    } );
    cost_t cand_cost = 0.0;
    foreach_output( n, [&]( auto const& f ) {
      auto const id = get_binding_index( f );
      ntk_.foreach_fanin( n, [&]( auto const& fi, auto ii ) {
        (void)fi;
        cand_cost = std::max( cand_cost, arrival_.get_time( new_children[ii] ) + lib.get_max_pin_delay( id, ii ) );
      } );
    } );

    return curr_cost - cand_cost;
  }

  cost_t evaluate( node_index_t const& n, std::vector<signal_t> const& children )
  {
    cost_t time = 0.0;
    foreach_output( n, [&]( auto const& f ) {
      time = std::max( time, arrival_.get_time( f ) );
    } );
    return time;
  }

  template<typename Fn>
  void foreach_gate( Fn&& fn )
  {
    if ( ps_.max_num_roots < std::numeric_limits<uint32_t>::max() )
    {
      sort_nodes();
      uint32_t const num_roots = std::min( ps_.max_num_roots, static_cast<uint32_t>( nodes_.size() ) );

      for ( uint32_t i = 0; i < num_roots; ++i )
      {
        node_index_t const n = nodes_[i].root;
        if ( !ntk_.is_dead( n ) && !ntk_.is_constant( n ) && !ntk_.is_pi( n ) )
          fn( n );
      }
    }
    else
    {
      ntk_.foreach_gate( [&]( node_index_t const& n ) {
        if ( !ntk_.is_dead( n ) && !ntk_.is_constant( n ) && !ntk_.is_pi( n ) )
          fn( n );
      } );
    }
  }

private:
  void compute_costs()
  {
    trackers::required_times_tracker<Ntk> required( ntk_, arrival_.compute_worst_delay() );
    ntk_.foreach_gate( [&]( auto const& n ) {
      double const node_cost = 0;
      ntk_.foreach_output( n, [&]( auto const& f ) {
        node_cost = std::max( node_cost, required.get_time( f ) - arrival_.get_time( f ) );
      } );
      nodes_[n] = { n, node_cost };
    } );
  }

  void sort_nodes()
  {
    nodes_.reserve( ntk_.size() );
    std::fill( nodes_.begin(), nodes_.end(), node_with_cost_t{} );
    compute_costs();
    std::stable_sort( nodes_.begin(), nodes_.end(), [&]( auto const& a, auto const& b ) {
      return a.mffc_delay > b.mffc_delay;
    } );
  }

private:
  Ntk& ntk_;
  evaluator_params const& ps_;
  std::vector<node_with_cost_t> nodes_;
  trackers::arrival_times_tracker<Ntk> arrival_;
};

} /* namespace evaluators */

} /* namespace opto */

} /* namespace mad_hatter */