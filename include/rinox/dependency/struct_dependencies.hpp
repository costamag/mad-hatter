/* rinox: C++ logic network library
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
  \file struct_dependencies.hpp
  \brief Compute dependencies allowing to window the pivot node.

  \author Andrea Costamagna
*/

#pragma once

#include "../math/math.hpp"
#include "dependency_cut.hpp"

namespace rinox
{

namespace dependency
{

struct default_struct_params
{
  static constexpr uint32_t num_vars_sign = 6u;
  static constexpr uint32_t max_cuts_size = 6u;
};

template<class Ntk, typename StaticParams = default_struct_params>
class struct_dependencies
{

public:
  static constexpr uint32_t num_vars_sign = StaticParams::num_vars_sign;
  static constexpr uint32_t max_cuts_size = StaticParams::max_cuts_size;
  using signal_t = typename Ntk::signal;
  using node_index_t = typename Ntk::node;
  using signature_t = kitty::static_truth_table<num_vars_sign>;

public:
  struct_dependencies( Ntk& ntk )
      : ntk_( ntk )
  {
  }

  template<typename WinMng, typename WinSim>
  void run( WinMng& window, WinSim& simulator )
  {
    assert( ( std::is_same<signature_t, typename WinSim::signature_t>::value && "signatures have different type" ) );
    cuts_.clear();

    window.mark_contained();
    // identify candidates through branch and bound
    structural_enumeration( window, simulator );
  }

  template<typename Fn>
  void foreach_cut( Fn&& fn )
  {
    for ( auto i = 0u; i < cuts_.size(); ++i )
    {
      fn( cuts_[i], i );
    }
  }

#pragma region Candidates Identification

  bool contains( std::vector<signal_t> const& cut1, std::vector<signal_t> const& cut2 )
  {
    if ( cut2.size() > cut1.size() )
      return false;
    bool contains = true;
    auto i = 0;
    auto j = 0;
    while ( contains && ( i < cut1.size() ) && ( j < cut2.size() ) )
    {
      if ( cut1[i] < cut2[j] )
        i++;
      else if ( cut1[i] == cut2[j] )
      {
        i++;
        j++;
      }
      else if ( cut1[i] > cut2[j] )
        contains = false;
    }
    if ( contains && ( j == cut2.size() ) )
      return true;
    return false;
  }

  bool contains( std::vector<signal_t> const& leaves, std::vector<std::vector<signal_t>> const& leaves_vec )
  {
    for ( auto const& other : leaves_vec )
    {
      if ( ( &leaves != &other ) && contains( leaves, other ) )
        return true;
    }
    return false;
  }

  template<typename WinMng>
  void structural_enumeration( std::vector<std::vector<signal_t>>& leaves_vec, std::vector<signal_t> const& leaves, WinMng& window )
  {
    for ( auto i = 0; i < leaves.size(); ++i )
    {
      // initialize the new cut
      std::vector<signal_t> new_leaves( leaves.size() - 1 );
      std::copy( leaves.cbegin(), leaves.cbegin() + i, new_leaves.begin() );
      std::copy( leaves.cbegin() + i + 1, leaves.cend(), new_leaves.begin() + i );
      // insert the fanins
      auto const n = ntk_.get_node( leaves[i] );
      if ( ntk_.is_pi( n ) )
        continue;

      bool abort = false;
      ntk_.foreach_fanin( n, [&]( auto fi, auto ii ) {
        if ( abort )
          return;
        if ( !window.is_contained( ntk_.get_node( fi ) ) )
        {
          abort = true;
          return;
        }
        uint32_t j = 0u;
        bool is_done = false;
        while ( !is_done && ( j < new_leaves.size() ) )
        {
          if ( new_leaves[j] > fi )
          {
            new_leaves.insert( new_leaves.begin() + j, fi );
            is_done = true;
          }
          else if ( new_leaves[j] == fi )
            is_done = true;
          else
            j++;
        }
        if ( j == new_leaves.size() )
          new_leaves.push_back( fi );
        return;
      } );
      if ( !abort && ( new_leaves.size() <= max_cuts_size ) )
      {
        if ( !contains( new_leaves, leaves_vec ) )
        {
          leaves_vec.push_back( new_leaves );
          structural_enumeration( leaves_vec, new_leaves, window );
        }
      }
    }
  }

  template<typename WinMng, typename WinSim>
  void structural_enumeration( WinMng const& window, WinSim& simulator )
  {
    std::vector<std::vector<signal_t>> cuts;
    cuts_.clear();
    auto const care = simulator.get_careset();
    std::vector<signal_t> leaves;
    auto const n = window.get_pivot();
    bool abort = false;

    ntk_.foreach_fanin( n, [&]( auto const& fi, auto ii ) {
      if ( window.is_contained( ntk_.get_node( fi ) ) )
        leaves.push_back( fi );
      else
        abort = true;
    } );
    if ( abort )
      return;

    std::stable_sort( leaves.begin(), leaves.end() );
    std::vector<std::vector<signal_t>> leaves_vec;

    structural_enumeration( leaves_vec, leaves, window );

    for ( auto const& leaves : leaves_vec )
    {
      if ( !contains( leaves, leaves_vec ) )
      {
        dependency_cut_t<Ntk, max_cuts_size> cut( dependency_t::STRUCT_DEP, n, leaves );
        std::vector<signature_t const*> in_ptrs;
        for ( auto const& l : leaves )
          in_ptrs.push_back( &simulator.get( l ) );
        ntk_.foreach_output( n, [&]( auto const& f ) {
          auto const func = extract_function<signature_t, max_cuts_size>( in_ptrs, simulator.get( f ), care );
          cut.add_func( func );
        } );
        cuts_.push_back( cut );
      }
    }
  }

#pragma endregion

private:
  Ntk& ntk_;
  std::vector<dependency_cut_t<Ntk, max_cuts_size>> cuts_;
};

} // namespace dependency

} // namespace rinox