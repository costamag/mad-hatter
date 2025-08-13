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

/*! \brief Compact signal representation with node index and output pin.
 *
 * This data structure represents a signal in the bound storage network.
 * It encodes both the node index and the output pin in a single 64-bit word,
 * enabling compact and efficient signal manipulation.
 *
 * The internal layout uses `NumBitsOutputs` bits for the output pin, and the
 * remaining `64 - NumBitsOutputs` bits for the node index.
 *
 * \note The layout of C++ bitfields is implementation-defined. This structure assumes
 * little-endian packing and is safe as long as it's not passed across ABI boundaries.
 *
 * \see rinox::bound::storage
 * \see rinox::bound::storage_node
 * \see rinox::bound::storage_types
 */

#pragma once

#include <cassert>
#include <cstdint>
#include <limits>
#include <vector>

namespace rinox
{

/*!
 * \namespace bound
 * \brief Types and utilities related to the bound network data structure.
 */
namespace network
{

/*! \brief Pointer to a node with output-pin specifier.
 *
 * This data structure contains the information to point to an output pin
 * of a node. The information is stored in an `uint64_t`, partitioned as
 * follows:
 * - `NumBitsOutputs` bits are used to indicate the output pin
 * - `64 - NumBitsOutputs` bits are used to specify the node index.
 *
 * Note: Bitfield layout is compiler-dependent; assumed to be packed in
 * little-endian order. This usage is safe as long as storage_signal is not
 * passed across ABI boundaries.
 *
 * \tparam NumBitsOutputs Number of bits for the output id
 */
template<int NumBitsOutputs>
struct storage_signal
{

  static_assert( NumBitsOutputs > 0 && NumBitsOutputs < 64,
                 "NumBitsOutputs must be between 1 and 63" );

public:
  /*! \brief Default constructor (zero-initialized) */
  constexpr storage_signal() = default;

  /*! \brief Constructs a signal from a node index and output pin */
  constexpr storage_signal( uint64_t index, uint64_t output ) : index( index ), output( output ) {}

  /*! \brief Constructs a signal from a packed 64-bit representation */
  constexpr storage_signal( uint64_t data ) : data( data ) {}

  union
  {
    /* the order is selected to ensure small numbers for hashing reasons */
    struct
    {
      uint64_t output : NumBitsOutputs;     // bits 1..NumBitsOutputs
      uint64_t index : 64 - NumBitsOutputs; // higher bits
    };
    uint64_t data;
  };

  /*! \brief Retrieves the node index portion */
  uint64_t get_index() const
  {
    return index;
  }
  /*! \brief Retrieves the output pin specifier */
  uint64_t get_output() const
  {
    return output;
  }

  /*! \brief Sets the node index */
  void set_index( uint64_t new_index )
  {
    index = new_index;
  }

  /*! \brief Sets the output pin specifier */
  void set_output( uint64_t new_output )
  {
    output = new_output;
  }

  /*! \brief Equality comparison based on packed 64-bit representation */
  constexpr bool operator==( storage_signal<NumBitsOutputs> const& other ) const
  {
    return data == other.data;
  }

  /*! \brief Inequality comparison */
  constexpr bool operator!=( storage_signal<NumBitsOutputs> const& other ) const
  {
    return data != other.data;
  }

  /*! \brief Converts signal to its packed 64-bit representation */
  constexpr operator uint64_t() const
  {
    return data;
  }
};

/*! \brief Hash function for bound nodes.
 *
 * This hash function combines the indices and output IDs of the node's children
 * and outputs to create a unique hash value for the node.
 */
template<uint32_t NumBitsOutputs>
struct signal_hash
{
  using signal_t = storage_signal<NumBitsOutputs>;

  template<class T>
  static inline void hash_combine( std::size_t& seed, T const& v )
  {
    seed ^= std::hash<T>{}( v ) + 0x9e3779b9 + ( seed << 6 ) + ( seed >> 2 );
  }

  std::size_t operator()( signal_t const& f ) const
  {
    std::size_t seed = 0;
    hash_combine( seed, f.output );
    hash_combine( seed, f.index );
    return seed;
  }
};

} // namespace network

} // namespace rinox
