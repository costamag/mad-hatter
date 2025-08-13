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
  \file signal_map.hpp
  \brief Map indexed by network signals

  \author Andrea Costamagna
*/

#pragma once

#include "../traits.hpp"

#include <cassert>
#include <memory>
#include <unordered_map>
#include <variant>
#include <vector>

#include <mockturtle/traits.hpp>

namespace rinox
{

namespace network
{

/*! \brief Vector-based signal map with validity query
 *
 * This container is a variant of the `incomplete_node_map` tailored for cases
 * in which different signals pointing to the same node should store different
 * values. A crucial use case is when signals contain bitfields for specifying
 * the output in a multiple output gate.
 *
 * The implementation uses a vector as underlying data structure, so
 * that it benefits from fast access. It is supplemented with an
 * additional validity field such that it can be used like an
 * `unordered_signal_map`.
 *
 * **Required network functions:**
 * - `signal_size`
 * - `signal_to_index`
 *
 */
template<class T, class Ntk>
class incomplete_signal_map
{
public:
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

  using container_type = std::vector<std::variant<std::monostate, T>>;

public:
  /*! \brief Default constructor. */
  explicit incomplete_signal_map( Ntk const& ntk )
      : ntk( &ntk ),
        data( std::make_shared<container_type>( ntk.signal_size() ) )
  {
    static_assert( !std::is_same_v<T, std::monostate>, "T cannot be std::monostate" );
    static_assert( mockturtle::is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( traits::has_signal_size_v<Ntk>, "Ntk does not implement the signal size method" );
    static_assert( traits::has_signal_to_index_v<Ntk>, "Ntk does not implement the signal_to_index method" );
  }

  /*! \brief Constructor with default value.
   *
   * Initializes all values in the container to `init_value`.
   */
  incomplete_signal_map( Ntk const& ntk, T const& init_value )
      : ntk( &ntk ),
        data( std::make_shared<container_type>( ntk.signal_size(), init_value ) )
  {
    static_assert( !std::is_same_v<T, std::monostate>, "T cannot be std::monostate" );
    static_assert( mockturtle::is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( traits::has_signal_size_v<Ntk>, "Ntk does not implement the signal size method" );
    static_assert( traits::has_signal_to_index_v<Ntk>, "Ntk does not implement the signal_to_index method" );
  }

  /*! \brief Number of keys stored in the data structure. */
  auto size() const
  {
    return data->size();
  }

  /*! \brief Check if a key is already defined. */
  bool has( signal const& f ) const
  {
    return std::holds_alternative<T>( ( *data )[ntk->signal_to_index( f )] );
  }

  /*! \brief Erase a key (if it exists). */
  void erase( signal const& f )
  {
    ( *data )[ntk->signal_to_index( f )] = std::monostate();
  }

  /*! \brief Mutable access to value by signal. */
  T& operator[]( signal const& f )
  {
    assert( ntk->signal_to_index( f ) < data->size() && "index out of bounds" );
    if ( !has( f ) )
    {
      ( *data )[ntk->signal_to_index( f )] = T();
    }
    return std::get<T>( ( *data )[ntk->signal_to_index( f )] );
  }

  /*! \brief Constant access to value by signal. */
  T const& operator[]( signal const& f ) const
  {
    assert( ntk->signal_to_index( f ) < data->size() && "index out of bounds" );
    assert( has( f ) );
    return std::get<T>( ( *data )[ntk->signal_to_index( f )] );
  }

  /*! \brief Resets the size of the map.
   *
   * This function should be called, if the network changed in size.  Then, the
   * map is cleared, and resized to the current network's size.  All values are
   * initialized with the place holder (empty) element.
   */
  void reset()
  {
    data->clear();
    data->resize( ntk->signal_size() );
  }

  /*! \brief Resets the size of the map.
   *
   * This function should be called, if the network changed in size.  Then, the
   * map is cleared, and resized to the current network's size.  All values are
   * initialized with `init_value`.
   *
   * \param init_value Initialization value after resize
   */
  void reset( T const& init_value )
  {
    data->clear();
    data->resize( ntk->signal_size(), init_value );
  }

  /*! \brief Resizes the map.
   *
   * This function should be called, if the signal_map's size needs to
   * be changed without clearing its data.
   */
  void resize()
  {
    if ( ntk->signal_size() > data->size() )
    {
      data->resize( ntk->signal_size() );
    }
  }

private:
  Ntk const* ntk;
  std::shared_ptr<container_type> data;
};

} /* namespace network */

} /* namespace rinox */