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
  \file bound_node.hpp
  \brief Defines the core node structure for the bound network.
  \details
    This file introduces `bound::storage_node`, a data structure representing a logic node
    within the bound storage network. Each node maintains information about its fan-in,
    fan-out, user-defined metadata, and a list of output pins.

    The template parameter `MaxNumOutputBits` controls the maximum number of outputs
    a node can support, enabling compatibility with multi-output standard cells.

    Nodes can be structurally compared and marked as "dead" by setting the type of their output pins.

  \ingroup bound_storage
  \see mad_hatter::bound::storage
  \see mad_hatter::bound::storage_signal
  \see mad_hatter::bound::storage_types

  \author Andrea Costamagna
*/

#pragma once

#include "storage_signal.hpp"
#include "utils.hpp"
#include <cstdint>
#include <vector>

namespace mad_hatter
{

/*!
 * \namespace bound
 * \brief Types and utilities related to the bound network data structure.
 */
namespace network
{

/*! \brief Node representation in the bound network.
 *
 * \tparam MaxNumOutputBits Maximum number of outputs supported per node.
 */
template<uint32_t MaxNumOutputBits>
struct storage_node
{
  using signal_t = storage_signal<MaxNumOutputBits>;

  /*! \brief Default constructor with a single (default) output. */
  storage_node()
  {
    outputs = decltype( outputs )( 1 );
  }

  /*! \brief Constructor that sets the pin type of the first output. */
  storage_node( pin_type_t const& type )
  {
    outputs = decltype( outputs )( 1 );
    outputs[0].type = type;
  }

  void clear()
  {
    user_data = 0;
    traversal_id = 0;
    fanout_count = 0;
    outputs.clear();
    children.clear();
  }

  /*! \brief Equality operator compares structural fan-in. */
  bool operator==( storage_node<MaxNumOutputBits> const& other ) const
  {
    return children == other.children;
  }

  /*! \brief Signals of the node's immediate fan-ins. */
  std::vector<signal_t> children;

  /*! \brief Custom user data for tagging or annotation. */
  uint32_t user_data{ 0 };

  /*! \brief Traversal marker used in graph algorithms. */
  uint32_t traversal_id{ 0 };

  /*! \brief Fan-out count; MSB may encode special flags (e.g., "dead"). */
  uint32_t fanout_count{ 0 };

  /*! \brief Output pins associated with this node. */
  std::vector<output_pin_t> outputs;
};

/*! \brief Hash function for bound nodes.
 *
 * This hash function combines the indices and output IDs of the node's children
 * and outputs to create a unique hash value for the node.
 */
template<uint32_t NumBitsOutputs>
struct node_hash
{
  using node_t = storage_node<NumBitsOutputs>;

  template<class T>
  static inline void hash_combine( std::size_t& seed, T const& v )
  {
    seed ^= std::hash<T>{}( v ) + 0x9e3779b9 + ( seed << 6 ) + ( seed >> 2 );
  }

  std::size_t operator()( node_t const& n ) const
  {
    std::size_t seed = 0;
    for ( auto const& child : n.children )
    {
      hash_combine( seed, child.index );
      hash_combine( seed, child.output );
    }
    for ( auto const& output : n.outputs )
    {
      hash_combine( seed, output.id );
    }
    return seed;
  }
};

} // namespace network

} // namespace mad_hatter