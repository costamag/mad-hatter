/* mad_hatter: C++ logic network library
 * Copyright (C) 2018-2022  EPFL
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
  \file window_manager.hpp
  \brief Construction of windows for mapped networks.

  \author Andrea Costamagna
*/

#pragma once

namespace mad_hatter
{

namespace windowing
{

template<class Ntk>
struct window_t
{
  using node_index_t = typename Ntk::node;
  using signal_t = typename Ntk::signal;

  node_index_t pivot;
  std::vector<node_index_t> tfos;
  std::vector<node_index_t> mffc;
  std::vector<signal_t> divs;
  std::vector<signal_t> outputs;
  std::vector<signal_t> inputs;
};

struct default_window_manager_params
{
  static constexpr uint32_t max_num_leaves = 8;
  uint32_t max_num_divisors = 128;
  bool preserve_depth = true;
  int32_t odc_levels = 0;
  uint32_t skip_fanout_limit_for_divisors{ 100 };
};

struct window_manager_stats
{
  bool valid = false;
};

template<class Ntk, typename Params = default_window_manager_params>
class window_manager
{
public:
  using node_index_t = typename Ntk::node;
  using signal_t = typename Ntk::signal;

public:
  window_manager( Ntk& ntk, Params const& ps, window_manager_stats& st )
      : ntk_( ntk ),
        ps_( ps ),
        st_( st )
  {
  }

  [[nodiscard]] bool run( node_index_t const& n )
  {
    st_.valid = false;
    init( n );

    // label the MFFC nodes
    collect_mffc_nodes();
    for ( auto const& m : window_.mffc )
    {
      ntk_.set_visited( m, ntk_.trav_id() );
      make_alien( m );
    }
    window_.mffc.clear();

    window_.inputs.clear();
    ntk_.foreach_output( n, [&]( auto const& f ) {
      window_.inputs.push_back( f );
      window_.divs.push_back( f );
    } );

    // expand the leaves until reaching the boundary of the MFFC
    expand_leaves( [&]( node_index_t const& v ) { return ( ntk_.visited( v ) == ntk_.trav_id() ) && !ntk_.is_pi( v ); },
                   [&]( node_index_t const& v ) { make_mffc( v ); } );

    collect_mffc_nodes();

    window_.divs = window_.inputs;
    /* expand toward the tfo */
    if ( ps_.odc_levels > 0 )
    {
      collect_nodes_tfo();
      // expand the divisors set to try removing upper-leaves with reconvergence
      collect_side_divisors();
    }
    else
    {
      ntk_.foreach_output( n, [&]( auto f ) {
        window_.outputs.push_back( f );
      } );
    }

    // expand the leaves to find reconvergences
    expand_leaves( [&]( node_index_t const& n ) { return !ntk_.is_pi( n ); },
                   [&]( node_index_t const& v ) { make_divisor( v ); } );

    // expand the divisors set to try removing upper-leaves with reconvergence
    collect_side_divisors();

    if ( ps_.odc_levels > 0 )
    {
      topological_sort( window_.tfos );
      topological_sort( window_.outputs );
    }
    topological_sort( window_.mffc );
    topological_sort( window_.divs );
    topological_sort( window_.inputs );

    st_.valid = true;
    return true;
  }

  std::vector<signal_t> const& get_divisors() const
  {
    return window_.divs;
  }

  signal_t const& get_divisor( uint32_t const& index ) const
  {
    return window_.divs[index];
  }

  std::vector<node_index_t> const& get_tfos() const
  {
    return window_.tfos;
  }

  std::vector<signal_t> const& get_outputs() const
  {
    return window_.outputs;
  }

  std::vector<signal_t> const& get_leaves() const
  {
    return window_.inputs;
  }

  std::vector<signal_t> const& get_divs() const
  {
    return window_.divs;
  }

  std::vector<node_index_t> const& get_mffc() const
  {
    return window_.mffc;
  }

  node_index_t const& get_pivot() const
  {
    return window_.pivot;
  }

  window_t<Ntk> get_window() const
  {
    return window_;
  }

#pragma region Miscellanea
private:
  void init( node_index_t const& n )
  {
    ntk_.incr_trav_id();
    window_.pivot = n;
    make_mffc( n );
    window_.outputs.clear();
    window_.tfos.clear();
    window_.mffc.clear();
    window_.divs.clear();
    window_.inputs.clear();
  }

  void topological_sort( std::vector<node_index_t>& nodes )
  {
    std::stable_sort( nodes.begin(), nodes.end(), [&]( auto const& a, auto const& b ) {
      return ntk_.level( a ) < ntk_.level( b );
    } );
  }

  void topological_sort( std::vector<signal_t>& signals )
  {
    std::stable_sort( signals.begin(), signals.end(), [&]( auto const& a, auto const& b ) {
      return ntk_.level( ntk_.get_node( a ) ) < ntk_.level( ntk_.get_node( b ) );
    } );
  }

public:
  bool is_tfo( node_index_t const& n ) const
  {
    return ntk_.value( n ) == ( 4u | ( ntk_.trav_id() << 3u ) );
  }

  void make_tfo( node_index_t const& n ) const
  {
    ntk_.set_value( n, ( 4u | ( ntk_.trav_id() << 3u ) ) );
  }

  bool is_contained( node_index_t const& n ) const
  {
    return ( ( ntk_.value( n ) >> 3u ) == ntk_.trav_id() );
  }

  bool is_output( node_index_t const& n ) const
  {
    return ntk_.value( n ) == ( 5u | ( ntk_.trav_id() << 3u ) );
  }

  void make_output( node_index_t const& n ) const
  {
    ntk_.set_value( n, ( 5u | ( ntk_.trav_id() << 3u ) ) );
  }

  void make_alien( node_index_t const& n ) const
  {
    ntk_.set_value( n, 0u );
  }

  bool is_leaf( node_index_t const& n ) const
  {
    return ntk_.value( n ) == ( 3u | ( ntk_.trav_id() << 3u ) );
  }

  void make_leaf( node_index_t const& n ) const
  {
    ntk_.set_value( n, ( 3u | ( ntk_.trav_id() << 3u ) ) );
  }

  bool is_mffc( node_index_t const& n ) const
  {
    return ntk_.value( n ) == ( 1u | ( ntk_.trav_id() << 3u ) );
  }

  void make_mffc( node_index_t const& n ) const
  {
    ntk_.set_value( n, ( 1u | ( ntk_.trav_id() << 3u ) ) );
  }

  bool is_divisor( node_index_t const& n ) const
  {
    return ntk_.value( n ) == ( 2u | ( ntk_.trav_id() << 3u ) );
  }

  void make_divisor( node_index_t const& n ) const
  {
    ntk_.set_value( n, ( 2u | ( ntk_.trav_id() << 3u ) ) );
  }

  bool is_valid()
  {
    return st_.valid;
  }
#pragma endregion

#pragma region TFO
  void collect_nodes_tfo()
  {
    std::vector<signal_t> outputs;
    std::vector<signal_t> inputs;
    std::vector<node_index_t> tfos;
    window_.tfos = {};
    window_.outputs.clear();
    ntk_.foreach_output( window_.pivot, [&]( auto const& f ) {
      window_.outputs.push_back( f );
    } );

    uint32_t num_leaves = window_.inputs.size();

    for ( auto lev = 0; lev < ps_.odc_levels; ++lev )
    {
      outputs.clear();
      inputs.clear();
      tfos.clear();
      for ( auto const& f : window_.outputs )
      {
        node_index_t n = ntk_.get_node( f );
        if ( !is_output( n ) && ( n != window_.pivot ) )
          continue;

        int cnt = 0;
        ntk_.foreach_fanout( n, [&]( auto no ) {
          if ( !is_tfo( no ) && !is_output( no ) )
          {
            ntk_.foreach_output( no, [&]( auto const& fo ) {
              outputs.push_back( fo );
              cnt++;
            } );
            make_output( no );
          }
        } );
        if ( cnt == 0 )
        {
          make_output( n );
          ntk_.foreach_output( n, [&]( auto const& fo ) {
            outputs.push_back( fo );
          } );
        }
        else if ( n != window_.pivot )
        {
          make_tfo( n );
          tfos.push_back( n );
        }
      }
      for ( signal_t const& f : outputs )
      {
        ntk_.foreach_fanin( f, [&]( auto const& fi, auto ii ) {
          auto const ni = ntk_.get_node( fi );
          if ( !is_contained( ni ) )
          {
            ntk_.foreach_output( ni, [&]( auto const& fo ) {
              inputs.push_back( fo );
            } );
            make_leaf( ni );
            num_leaves += ntk_.num_outputs( ni );
          }
        } );
      }
      if ( num_leaves <= ps_.max_num_leaves )
      {
        window_.outputs = outputs;
        for ( auto const& f : inputs )
        {
          window_.inputs.push_back( f );
          window_.divs.push_back( f );
        }
        for ( auto const& n : tfos )
          window_.tfos.push_back( n );
      }
      else
      {
        for ( auto const& f : inputs )
          make_alien( ntk_.get_node( f ) );
        for ( auto const& f : window_.outputs )
          make_output( ntk_.get_node( f ) );
      }
    }
  }
#pragma endregion

#pragma region Leaves
  template<typename DoExpandFn, typename Apply>
  void expand_leaves( DoExpandFn&& do_expand, Apply&& apply )
  {
    while ( true )
    {
      int best_cost = ps_.max_num_leaves - window_.inputs.size() + 1;
      std::optional<node_index_t> best_node = std::nullopt;
      for ( auto const& l : window_.inputs )
      {
        node_index_t leaf = ntk_.get_node( l );
        if ( do_expand( leaf ) )
        {
          int cost = compute_leaf_cost( leaf );
          if ( cost < best_cost )
          {
            best_cost = cost;
            best_node = std::make_optional( leaf );
          }
        }
      }
      if ( best_node )
      {
        ntk_.foreach_fanin( *best_node, [&]( auto const& fi, auto ii ) {
          auto const ni = ntk_.get_node( fi );
          if ( !is_contained( ni ) )
          {
            ntk_.foreach_output( ni, [&]( auto const& fo ) {
              window_.inputs.push_back( fo );
              window_.divs.push_back( fo );
            } );
            make_leaf( ni );
          }
        } );
        auto& leaves = window_.inputs;
        apply( *best_node );
        ntk_.foreach_output( *best_node, [&]( auto const& f ) {
          leaves.erase( std::remove( leaves.begin(), leaves.end(), f ), leaves.end() );
        } );
      }
      else
      {
        return;
      }
    }
    return;
  }

  int compute_leaf_cost( node_index_t const& n )
  {
    if ( ntk_.is_pi( n ) )
      return ps_.max_num_leaves;

    int cost = -static_cast<int>( ntk_.num_outputs( n ) );
    ntk_.foreach_fanin( n, [&]( auto const& fi, auto ii ) {
      auto const ni = ntk_.get_node( fi );
      if ( !is_contained( ni ) )
        cost += ntk_.num_outputs( ni );
    } );
    return cost;
  }
#pragma endregion Leaves

#pragma region Mffc
  void collect_mffc_nodes()
  {
    /* dereference the node_index_t */
    window_.mffc.clear();
    make_mffc( window_.pivot );
    window_.mffc = { window_.pivot };

    for ( auto const& l : window_.inputs )
      ntk_.incr_fanout_size( ntk_.get_node( l ) );

    node_deref_rec( window_.pivot );
    node_ref_rec( window_.pivot );

    for ( auto const& l : window_.inputs )
      ntk_.decr_fanout_size( ntk_.get_node( l ) );
  }

  /* ! \brief Dereference the node's MFFC */
  void node_deref_rec( node_index_t const& n )
  {
    if ( ntk_.is_pi( n ) )
      return;
    ntk_.foreach_fanin( n, [&]( const auto& f ) {
      auto const& p = ntk_.get_node( f );
      if ( ntk_.is_pi( p ) )
        return;
      ntk_.decr_fanout_size( p );
      if ( ntk_.fanout_size( p ) == 0 )
      {
        make_mffc( p );
        window_.mffc.push_back( p );
        node_deref_rec( p );
      }
    } );
  }

  /* ! \brief Reference the node's MFFC */
  void node_ref_rec( node_index_t const& n )
  {
    if ( ntk_.is_pi( n ) )
      return;
    ntk_.foreach_fanin( n, [&]( const auto& f ) {
      auto const& p = ntk_.get_node( f );
      if ( ntk_.is_pi( p ) )
        return;
      auto v = ntk_.fanout_size( p );
      ntk_.incr_fanout_size( p );
      if ( v == 0 && !ntk_.is_pi( p ) )
      {
        node_ref_rec( p );
      }
    } );
  }
#pragma endregion

#pragma region Divisors
  void collect_side_divisors()
  {
    uint32_t max_level = std::numeric_limits<uint32_t>::min();
    for ( auto const& f : window_.outputs )
      max_level = std::max( max_level, ntk_.level( ntk_.get_node( f ) ) );

    bool done = false;
    while ( !done )
    {
      done = true;
      // check if any leaf is contained by other leaves
      for ( auto const& f : window_.inputs )
      {
        auto const n = ntk_.get_node( f );
        if ( is_leaf( n ) && !ntk_.is_pi( n ) )
        {
          bool contained = true;
          ntk_.foreach_fanin( n, [&]( auto const& fi, auto ii ) {
            auto const ni = ntk_.get_node( fi );
            contained &= is_contained( ni );
          } );
          if ( contained )
          {
            make_divisor( n );
          }
        }
      }

      auto& inputs = window_.inputs;
      inputs.erase( std::remove_if( inputs.begin(),
                                    inputs.end(),
                                    [&]( signal_t const& f ) { return is_divisor( ntk_.get_node( f ) ); } ),
                    inputs.end() );

      std::vector<signal_t> new_divs;
      for ( auto const& f : window_.divs )
      {
        auto const n = ntk_.get_node( f );
        ntk_.foreach_fanout( n, [&]( auto const& no ) {
          if ( is_contained( no ) )
            return;
          if ( ps_.preserve_depth && ( ntk_.level( no ) >= max_level ) )
            return;
          bool add = true;
          ntk_.foreach_fanin( no, [&]( auto const& fi, auto ii ) {
            auto ni = ntk_.get_node( fi );
            add &= is_contained( ni );
          } );
          if ( add )
          {
            make_divisor( no );
            ntk_.foreach_output( no, [&]( auto const& fo ) {
              new_divs.push_back( fo );
            } );
            done = false;
          }
          return;
        } );
      }
      for ( auto d : new_divs )
        window_.divs.push_back( d );
    }
  }

public:
  size_t num_inputs() const
  {
    return window_.inputs.size();
  }

  size_t num_outputs() const
  {
    return window_.outputs.size();
  }

  size_t num_divisors() const
  {
    return window_.divs.size();
  }

  size_t size() const
  {
    return window_.divs.size() + window_.outputs.size() + ( window_.tfos.size() * Ntk::max_num_outputs );
  }

  /*! \brief iterator to apply a lambda to all the nodes */
  template<typename Fn>
  void foreach_input( Fn&& fn ) const
  {
    for ( uint32_t i = 0; i < window_.inputs.size(); i++ )
    {
      fn( window_.inputs[i], i );
    }
  }

  /*! \brief iterator to apply a lambda to all the nodes */
  template<typename Fn>
  void foreach_divisor( Fn&& fn ) const
  {
    for ( uint32_t i = 0; i < window_.divs.size(); i++ )
    {
      fn( window_.divs[i], i );
    }
  }

  /*! \brief iterator to apply a lambda to all the nodes */
  template<typename Fn>
  void foreach_mffc( Fn&& fn ) const
  {
    for ( uint32_t i = 0; i < window_.mffc.size(); i++ )
    {
      fn( window_.mffc[i], i );
    }
  }

  /*! \brief iterator to apply a lambda to all the nodes */
  template<typename Fn>
  void foreach_tfo( Fn&& fn ) const
  {
    for ( uint32_t i = 0; i < window_.tfos.size(); i++ )
    {
      fn( window_.tfos[i], i );
    }
  }

  /*! \brief iterator to apply a lambda to all the nodes */
  template<typename Fn>
  void foreach_output( Fn&& fn ) const
  {
    for ( uint32_t i = 0; i < window_.outputs.size(); i++ )
    {
      fn( window_.outputs[i], i );
    }
  }

private:
  Ntk& ntk_;
  window_t<Ntk> window_;
  Params const& ps_;
  window_manager_stats& st_;
};

} // namespace windowing

} // namespace mad_hatter