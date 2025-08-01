/* mad_hatter: C++ logic network library
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
  \file chain_simulator.hpp
  \brief Simulator engine for index chains.

  \author Andrea Costamagna
*/

#pragma once

#include "boolean_chains.hpp"

#include <algorithm>
#include <vector>

namespace mad_hatter
{

namespace evaluation
{

/*! \brief Simulator engine for XAG, AIG, and MIG-index chains.
 *
 * This engine can be used to efficiently simulate many index chains.
 * In this context, a simulation pattern is a truth table corresponding
 * to a node’s Boolean vector under the given input assignments.
 * The simulator pre-allocates the memory necessary to store the simulation patterns
 * of an index chain, and extends this memory when the evaluator is required
 * to perform Boolean evaluation of a chain that is larger than the current capacity
 * of the simulator. To avoid unnecessary copies, the input simulation patterns
 * must be passed as a vector of raw pointers.
 *
 * \tparam chain Type of the index chain to be simulated.
 * \tparam TT Truth table type.
 *
 * \verbatim embed:rst

   Example

   .. code-block:: c++

      using chain_t = large_xag_boolean_chain;
      using truth_table_t = kitty::static_truth_table<4u>;
      chain_simulator<chain_t, truth_table_t> sim;
      sim( chain1, inputs1 );
      sim( chain2, inputs2 );
   \endverbatim
 */
template<typename Chain, typename TT>
class chain_simulator
{
public:
  /* type of the literals of the index chain */
  using element_type = typename Chain::element_type;

  chain_simulator()
      : const0( TT().construct() )
  {
    /* The value 20 is an upper-bound of the size of most practical chains */
    sims.resize( 20u );
  }

  /*! \brief Simulate the chain in topological order.
   *
   * This method updates the internal state of the simulator by
   * storing in `sims` the simulation patterns of the nodes in the chain.
   *
   * \param chain Index chain to be simulated.
   * \param inputs Vector of TT raw pointers to the input simulation patterns.
   */
  void operator()( Chain const& chain, std::vector<TT const*> const& inputs )
  {
    /* update the allocated memory */
    if ( sims.size() < chain.num_gates() )
      sims.resize( std::max<size_t>( sims.size(), chain.num_gates() ) ); /* ensure that the constant 0 simulation is correct ( for dynamic truth tables ) */
    if ( ( inputs.size() ) > 0 && ( const0.num_bits() != inputs[0]->num_bits() ) )
      const0 = inputs[0]->construct();
    /* traverse the chain in topological order and simulate each node */
    size_t i = 0;
    if constexpr ( std::is_same<Chain, xag_boolean_chain<true>>::value || std::is_same<Chain, xag_boolean_chain<false>>::value )
    {
      chain.foreach_gate( [&]( element_type const& lit_lhs, element_type const& lit_rhs ) {
        auto const [tt_lhs_ptr, is_lhs_compl] = get_simulation( chain, inputs, lit_lhs );
        auto const [tt_rhs_ptr, is_rhs_compl] = get_simulation( chain, inputs, lit_rhs );
        sims[i++] = chain.is_and( lit_lhs, lit_rhs )
                        ? complement( *tt_lhs_ptr, is_lhs_compl ) & complement( *tt_rhs_ptr, is_rhs_compl )
                        : complement( *tt_lhs_ptr, is_lhs_compl ) ^ complement( *tt_rhs_ptr, is_rhs_compl );
      } );
    }
    else if constexpr ( std::is_same<Chain, mig_boolean_chain>::value )
    {
      chain.foreach_gate( [&]( element_type const& lit0, element_type const& lit1, element_type const& lit2 ) {
        auto const [tt_0_ptr, is_0_compl] = get_simulation( chain, inputs, lit0 );
        auto const [tt_1_ptr, is_1_compl] = get_simulation( chain, inputs, lit1 );
        auto const [tt_2_ptr, is_2_compl] = get_simulation( chain, inputs, lit2 );
        sims[i++] = maj( complement( *tt_0_ptr, is_0_compl ), complement( *tt_1_ptr, is_1_compl ), complement( *tt_2_ptr, is_2_compl ) );
      } );
    }
  }

  /*! \brief Invert a truth table if needed.
   *
   *  Defined for readability.
   *
   * \param tt Truth table.
   * \param is_compl True when the truth table should be complemented.
   * \return The input truth table complemented when `is_compl=true`.
   */
  inline TT complement( TT const& tt, bool is_compl )
  {
    return is_compl ? ~tt : tt;
  }

  /*! \brief Compute the majority function of three truth tables.
   *
   * \param tt0 First truth table.
   * \param tt1 Second truth table.
   * \param tt2 Third truth table.
   * \return Majority-of-3 of `tt0`, `tt1`, and `tt2`.
   */
  inline TT maj( TT const& tt0, TT const& tt1, TT const& tt2 )
  {
    return ( tt0 & tt1 ) | ( tt0 & tt2 ) | ( tt1 & tt2 );
  }

  /*! \brief Return the simulation associated to the literal
   *
   * \param chain An XAIG index chain, with or without separated header.
   * \param inputs A vector of pointers to the input simulation patterns.
   * \param lit The literal whose simulation we want to extract.
   *
   * The size of `inputs` should be equal to the number of inputs of the chain.
   * Keep private to avoid giving external access to memory that could be later corrupted.
   *
   * \return A tuple containing a pointer to the simulation pattern and a flag for complementation.
   */
  [[nodiscard]] std::tuple<TT const*, bool> get_simulation( Chain const& chain, std::vector<TT const*> const& inputs, element_type const& lit )
  {
    if ( chain.is_constant( lit ) )
    {
      return { &const0, chain.is_complemented( lit ) };
    }
    if ( chain.num_pis() > inputs.size() )
      throw std::invalid_argument( "Mismatch between number of PIs and input simulations." );
    if ( chain.is_pi( lit ) )
    {
      uint32_t index = chain.get_pi_index( lit );
      TT const& sim = *inputs[index];
      return { &sim, chain.is_complemented( lit ) };
    }
    uint32_t index = chain.get_node_index( lit );
    TT const& sim = sims[index];
    return { &sim, chain.is_complemented( lit ) };
  }

  /*! \brief Extract the simulation of a literal
   *
   * \param res Truth table where to store the result.
   * \param chain Index chain to be simulated.
   * \param inputs Vector of pointers to the input truth tables.
   * \param lit Literal whose simulation we want to extract.
   */
  inline void get_simulation_inline( TT& res, Chain const& chain, std::vector<TT const*> const& inputs, element_type const& lit )
  {
    auto const [tt, is_compl] = get_simulation( chain, inputs, lit );
    res = is_compl ? ~( *tt ) : *tt;
  }

private:
  /*! \brief Simulation of the internal nodes ( no inputs and constants ) */
  std::vector<TT> sims;
  /*! \brief Constant 0 simulation */
  TT const0;

}; /* chain simulator */

} /* namespace evaluation*/

} /* namespace mad_hatter */