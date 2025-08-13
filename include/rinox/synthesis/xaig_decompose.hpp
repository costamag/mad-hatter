/* rinox: C++ logic network library
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
  \file xaig_decompose.hpp
  \brief Synthesis engine for XAG index lists.

  This header defines an engine for synthesizing XAG index lists from
  incompletely specified Boolean functions. The engine employs a recursive
  procedure with the following steps:

  - Minimize the functional support
  - If (support size ≤ 4)
      - Perform Boolean matching with don't-cares using a database
  - Else
      - Perform a support-reducing decomposition step

  \todo Improve the synthesis engine by implementing techniques from:
    - "An Enhanced Resub. Algorithm for Area-Oriented Logic Optimization".
    - "Symmetry-Based Synthesis for Interpretable Boolean Evaluation".

  \author Andrea Costamagna
*/

#pragma once

#include "../databases/database_manager.hpp"
#include "../evaluation/chains/xag_chain.hpp"
#include <mockturtle/algorithms/node_resynthesis/xag_npn.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/utils/network_utils.hpp>
#include <mockturtle/utils/stopwatch.hpp>

#include <kitty/kitty.hpp>

#include <chrono>
#include <type_traits>

namespace rinox
{

namespace synthesis
{

struct xaig_decompose_stats
{
  /*! \brief Time for finding 0-resub and collecting unate literals. */
  mockturtle::stopwatch<>::duration time_lookup{ 0 };

  /*! \brief Time for selecting the variable forn the division. */
  mockturtle::stopwatch<>::duration time_varsel{ 0 };

  /*! \brief Time for dividing the target and recursive call. */
  mockturtle::stopwatch<>::duration time_divide{ 0 };

  void report() const
  {
    fmt::print( "[i]         <xaig_decompose>\n" );
    fmt::print( "[i]             look-up             : {:>5.2f} secs\n",
                mockturtle::to_seconds( time_lookup ) );
    fmt::print( "[i]             variable selection  : {:>5.2f} secs\n",
                mockturtle::to_seconds( time_varsel ) );
    fmt::print( "[i]             division            : {:>5.2f} secs\n",
                mockturtle::to_seconds( time_divide ) );
  }
};

/*! \brief Logic synthesis engine for AIGs or XAGs.
 *
 * Combine functional decomposition and database-based synthesis.
 *
 * It accepts an incompletely specified Boolean function, represented as a
 * ternary truth table. The function is synthesized by recursively applying
 * support-reducing decompositions until the support size is reduced to ≤ 4
 * variables. At that point, synthesis is completed via a database lookup.
 *
 * \tparam UseXors If true, XOR gates are allowed in the index list.
 * \tparam UseDCs If true, Boolean matching exploits don't cares.
 *
 * \verbatim embed:rst

   **Example**

   .. code-block:: c++

      constexpr uint32_t NumVars = ...;
      using TT = kitty::static_truth_table<NumVars>;
      const TT onset = ..., careset = ...;
      kitty::ternary_truth_table<TT> func( onset, careset );
      xaig_decompose_stats st;
      xaig_decompose resyn( st );
      auto result = resyn( func );

   \endverbatim
 */
template<bool UseDCs = false, bool UseXors = false>
class xaig_decompose
{
  using stats = xaig_decompose_stats;
  using chain_t = rinox::evaluation::chains::large_xag_chain;
  using element_type_t = chain_t::element_type;
  using Ntk = typename std::conditional<UseXors, mockturtle::xag_network, mockturtle::aig_network>::type;
  using signal = typename Ntk::signal;
  using node = typename Ntk::node;
  static constexpr mockturtle::xag_npn_db_kind database_t = UseXors ? mockturtle::xag_npn_db_kind::xag_complete : mockturtle::xag_npn_db_kind::aig_complete;

  /*! \brief Types of support-reducing decompositions.
   */
  enum class decomp_t : uint8_t
  {
    AND, // F =  x & F1
    XOR, // F =  x ^ F0
    LT,  // F = !x & F0
    LE,  // F = !x | F1
    GE,  // F =  x | F0
    ITE  // F = ite( x, F1, F0 )
  };

public:
  explicit xaig_decompose( stats& st ) noexcept
      : database(), st( st )
  {
  }

  /*! \brief Perform XAIG synthesis from incompletely specified functions.
   *
   * Reset the internal index list and invokes the recursive synthesis engine
   * to construct a logic network from a given incompletely specified function.
   *
   * \param func The incompletely specified Boolean function, represented as a
   *             `kitty::ternary_truth_table`.
   *
   * \tparam TT The type of truth table used to represent the onset and offset
   *            of the Boolean function (e.g., `kitty::static_truth_table<N>`).
   */
  template<typename TT>
  void operator()( kitty::ternary_truth_table<TT> const& func )
  {
    /* reset the internal index list to the new synthesis problem */
    chain.clear();
    auto const num_vars = func.num_vars();
    chain.add_inputs( num_vars );

    /* initialize the support with the literals */
    std::vector<element_type_t> support( num_vars );
    uint32_t i = 1u;
    std::generate( support.begin(), support.end(), [&i]() { return i++ << 1; } );

    /* call the synthesis engine recursively */
    element_type_t const lit = recursive_synthesis( support, func );
    chain.add_output( lit );
  }

  /*! \brief Perform XAIG synthesis from completely specified functions.
   *
   * Defines the corresponding incompletely specified function and falls back
   * to the corresponding method.
   *
   * \param tt The completely specified Boolean function.
   *
   * \tparam TT The type of truth table used to represent the onset.
   */
  template<typename TT>
  void operator()( TT const& tt )
  {
    kitty::ternary_truth_table<TT> func( tt );
    return ( *this )( func );
  }

  /*! \brief Getter to obtain the last index list synthesized by the engine.
   */
  chain_t const& get_chain() const
  {
    return chain;
  }

private:
  /*! \brief Core synthesis engine.
   *
   * Manage the synthesis action based on the size of the functional support.
   *
   * \param support Vector of literals from the index list inputs.
   * \param func Incompletely specified function ( `ternary_truth_table` ).
   *
   * \tparam TT The type of truth table used to represent the onset and offset
   *            of the Boolean function (e.g., `kitty::static_truth_table<N>`).
   */
  template<typename TT>
  element_type_t recursive_synthesis( std::vector<element_type_t> const& support,
                                      kitty::ternary_truth_table<TT> func )
  {
    /* determine the functional support */
    func._bits &= func._care;
    auto supp = kitty::min_base_inplace<TT, UseDCs>( func );
    size_t supp_size = supp.size();

    /* when the support size is 0 the function is a constant */
    if ( supp_size == 0 )
    {
      TT const tt = func._bits & func._care;
      return chain.get_constant( !kitty::is_const0( tt ) );
    }

    /* collect the new support */
    std::vector<uint32_t> new_support;
    for ( auto s : supp )
    {
      new_support.push_back( support[s] );
    }

    /* database-based look-up available for ≤ 4 variables */
    if ( supp_size <= 4u )
    {
      auto const lit = mockturtle::call_with_stopwatch( st.time_lookup, [&]() {
        return boolean_matching( new_support, func );
      } );
      return lit;
    }
    /* variable selection for the decomposition */
    auto const varsel_result = mockturtle::call_with_stopwatch( st.time_varsel, [&]() {
      return choose_variable( func, supp_size );
    } );

    auto const& index = std::get<0>( varsel_result );
    auto const& op = std::get<1>( varsel_result );

    /* support-reducing decomposition */
    auto const lit = mockturtle::call_with_stopwatch( st.time_divide, [&]() {
      return decompose( new_support, index, op, func );
    } );
    return lit;
  }

  /*! \brief Returns the variable resulting in a compact decomposition.
   *
   * Iterate over the variables in the functional support and returns a
   * variable if the function is decomposable in that variable using a 2-inputs
   * Boolean operator. If such a decomposition doesn't exist, the variable
   * selector relies on an heuristic to assign a cost to each variable, and
   * and returns the variable with the lowest cost, to be used for a Shannon
   * decomposition.
   *
   * \param func Incompletely specified function ( `ternary_truth_table` ).
   * \param supp_size Number of variables in the functional support.
   *
   * \tparam TT The type of truth table used to represent the onset and offset
   *            of the Boolean function (e.g., `kitty::static_truth_table<N>`).
   */
  template<typename TT>
  std::tuple<uint32_t, decomp_t> choose_variable(
      kitty::ternary_truth_table<TT> const& func,
      int supp_size )
  {
    uint32_t cost, min_cost = std::numeric_limits<uint32_t>::max();
    int best_index = -1;
    for ( int i = supp_size - 1; i >= 0; --i )
    {
      auto tt0 = kitty::cofactor0( func, i );
      auto tt1 = kitty::cofactor1( func, i );
      if constexpr ( UseXors )
      {
        if ( kitty::equal<TT, UseDCs>( tt0, ~tt1 ) ) // F =  x ^ F0
        {
          return { i, decomp_t::XOR };
        }
      }
      if ( kitty::is_const0<TT, UseDCs>( tt0 ) ) // F =  x & F1
      {
        return { i, decomp_t::AND };
      }
      else if ( kitty::is_const0<TT, UseDCs>( tt1 ) ) // F = !x & F0
      {
        return { i, decomp_t::LT };
      }
      else if ( kitty::is_const0<TT, UseDCs>( ~tt0 ) ) // F = !x | F1
      {
        return { i, decomp_t::LE };
      }
      else if ( kitty::is_const0<TT, UseDCs>( ~tt1 ) ) // F =  x | F0
      {
        return { i, decomp_t::GE };
      }
      cost = kitty::count_ones( tt0 ) * kitty::count_ones( tt1 );
      if ( cost < min_cost )
      {
        min_cost = cost;
        best_index = i;
      }
    }
    assert( best_index >= 0 );
    // F = ite( x, F1, F0 )
    return { best_index, decomp_t::ITE };
  }

  /*! \brief Performs a specified support-reducing decomposition.
   *
   * \param support Literals in teh functional support.
   * \param index index of the variable to use in the decomposition.
   * \param op decomposition type.
   * \param func incompletely specified Boolean function to synthesize.
   *
   * \tparam TT The type of truth table used to represent the onset and offset
   *            of the Boolean function (e.g., `kitty::static_truth_table<N>`).
   */
  template<typename TT>
  element_type_t decompose( std::vector<element_type_t> const& support,
                            uint32_t index,
                            decomp_t op,
                            kitty::ternary_truth_table<TT> const& func )
  {
    element_type_t lit_fun, lit_var = support[index];
    kitty::ternary_truth_table<TT> tt0, tt1;

    switch ( op )
    {
    case decomp_t::AND: // F =  x & F1
    {
      tt1 = kitty::cofactor1( func, index );
      lit_fun = recursive_synthesis( support, tt1 );
      return chain.add_and( lit_var, lit_fun );
    }
    case decomp_t::LT: // F = !x & F0
    {
      tt0 = kitty::cofactor0( func, index );
      lit_fun = recursive_synthesis( support, tt0 );
      return chain.add_and( chain.add_not( lit_var ), lit_fun );
    }
    case decomp_t::LE: // F = !x | F1
    {
      tt1 = kitty::cofactor1( func, index );
      lit_fun = recursive_synthesis( support, tt1 );
      return chain.add_or( chain.add_not( lit_var ), lit_fun );
    }
    case decomp_t::GE: // F =  x | F0
    {
      tt0 = kitty::cofactor0( func, index );
      lit_fun = recursive_synthesis( support, tt0 );
      return chain.add_or( lit_var, lit_fun );
    }
    case decomp_t::XOR: // F =  x ^ F0
    {
      tt0 = kitty::cofactor0( func, index );
      tt1 = kitty::cofactor1( func, index );
      kitty::ternary_truth_table<TT> ttt;
      ttt._care = tt0._care | tt1._care;
      ttt._bits = ttt._care & ( tt0._bits & ~tt1._bits );
      lit_fun = recursive_synthesis( support, ttt );
      return chain.add_xor( lit_var, lit_fun );
    }
    case decomp_t::ITE: // F = ite( x, F1, F0 )
    {
      tt0 = kitty::cofactor0( func, index );
      tt1 = kitty::cofactor1( func, index );
      auto lit_fn0 = recursive_synthesis( support, tt0 );
      auto lit_fn1 = recursive_synthesis( support, tt1 );
      auto lit_cf0 = chain.add_and( chain.add_not( lit_var ), lit_fn0 );
      auto lit_cf1 = chain.add_and( lit_var, lit_fn1 );
      return chain.add_or( lit_cf0, lit_cf1 );
    }
    }
    return std::numeric_limits<element_type_t>::max();
  }

  /*! \brief Synthesis step based on database look-up.
   *
   * When the support size is ≤ 4 this method manages Boolean matching ( with
   * don't cares if `UseDCs=true` ) to synthesize the function using the
   * precomputed structure in the database.
   *
   * \param support Functional support.
   * \param func Incompletely specified function ( `ternary_truth_table` ).
   *
   * \tparam TT The type of truth table used to represent the onset and offset
   *            of the Boolean function (e.g., `kitty::static_truth_table<N>`).
   */
  template<typename TT>
  element_type_t boolean_matching( std::vector<element_type_t> const& support,
                                   kitty::ternary_truth_table<TT> func )
  {
    /* make the truth table representation compatibile with the database */
    kitty::ternary_truth_table<kitty::static_truth_table<4u>> tt;
    if ( func.num_vars() > 4 )
      kitty::shrink_to_inplace( tt, func );
    else
      kitty::extend_to_inplace( tt, func );

    /* extract the sub-networks implementing the functionality */
    auto info = database.lookup_npn( tt );
    size_t cost, min_cost = std::numeric_limits<size_t>::max();
    typename Ntk::signal best_sign;
    assert( info );
    /* identify the best sub-network */
    info->foreach_entry( [&]( auto f ) {
      cost = database.get_cost( f );
      if ( cost < min_cost )
      {
        min_cost = cost;
        best_sign = f;
      }
    } );

    /* insert the sub-network in the index list */
    auto lit_out = database.insert( *info,
                                    chain,
                                    best_sign,
                                    support.begin(),
                                    support.end() );
    return lit_out;
  }

private:
  /*! \brief Global index list synthesized by a run of the engine. */
  chain_t chain;
  /*! \brief Manager encapsulating the operations on the database */
  databases::database_manager<Ntk, UseDCs> database;

  stats& st;
};

} /* namespace synthesis */

} /* namespace rinox */