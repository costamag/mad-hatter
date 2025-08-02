

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
  \file traits.hpp
  \brief Type traits and checkers for the network interface

  \author Andrea Costamagna
*/

#pragma once

namespace mad_hatter
{

namespace traits
{

/*! \brief Conditiional false value for static assertions.
 */
template<typename Ntk>
struct dependent_false : std::false_type
{
};

template<class T>
struct is_boolean_chain : std::false_type
{
};

template<>
struct is_boolean_chain<mad_hatter::evaluation::chains::xag_chain<true>> : std::true_type
{
};

template<>
struct is_boolean_chain<mad_hatter::evaluation::chains::xag_chain<false>> : std::true_type
{
};

template<>
struct is_boolean_chain<mad_hatter::evaluation::chains::mig_chain> : std::true_type
{
};

template<class T>
inline constexpr bool is_boolean_chain_v = is_boolean_chain<T>::value;

#pragma region has_signal_size
template<class Ntk, class = void>
struct has_signal_size : std::false_type
{
};

template<class Ntk>
struct has_signal_size<Ntk, std::void_t<decltype( std::declval<Ntk>().signal_size() )>> : std::true_type
{
};

template<class Ntk>
inline constexpr bool has_signal_size_v = has_signal_size<Ntk>::value;
#pragma endregion

#pragma region has_signal_to_index
template<class Ntk, class = void>
struct has_signal_to_index : std::false_type
{
};

template<class Ntk>
struct has_signal_to_index<Ntk, std::void_t<decltype( std::declval<Ntk>().signal_to_index( std::declval<mockturtle::signal<Ntk>>() ) )>> : std::true_type
{
};

template<class Ntk>
inline constexpr bool has_signal_to_index_v = has_signal_to_index<Ntk>::value;
#pragma endregion

} /* namespace traits */

} // namespace mad_hatter