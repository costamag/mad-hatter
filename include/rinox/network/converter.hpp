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
template<typename NtkSrc, design_type_t DesignType = design_type_t::CELL_BASED, uint32_t MaxNumOutputs = 2u>
class converter
{
  static constexpr design_type_t design_t = DesignType;
  static constexpr uint32_t max_num_outputs = MaxNumOutputs;
  using NtkDst = bound_network<design_t, max_num_outputs>;

private:
  NtkSrc& ntk_src;
  NtkDst ntk_dst;
};

} // namespace network

} // namespace rinox
