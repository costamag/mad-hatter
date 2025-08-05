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
  \file lut_decomposer.hpp
  \brief Don't care-aware LUT-decomposition

  \author Andrea Costamagna
*/

#pragma once

#include "../dependency/dependency_cut.hpp"
#include <fmt/format.h>
#include <kitty/static_truth_table.hpp>

namespace mad_hatter
{

namespace synthesis
{

template<uint32_t NumVars = 6u>
struct spec_t
{
  using func_t = kitty::ternary_truth_table<kitty::static_truth_table<NumVars>>;

  spec_t() = default;

  spec_t( func_t const& sim )
      : sim( sim )
  {}

  spec_t( std::vector<uint8_t> const& inputs, func_t const& sim )
      : inputs( inputs ), sim( sim )
  {}

  std::vector<uint8_t> inputs;
  func_t sim;
};

template<uint32_t MaxCutSize = 6u, uint32_t MaxNumVars = 6u>
class lut_decomposer
{
public:
  using specs_t = std::vector<spec_t<MaxCutSize>>;
  using cut_func_t = kitty::static_truth_table<MaxCutSize>;
  using dat_func_t = kitty::static_truth_table<MaxNumVars>;
  using incomplete_cut_func_t = kitty::ternary_truth_table<cut_func_t>;
  using incomplete_dat_func_t = kitty::ternary_truth_table<dat_func_t>;

public:
  lut_decomposer()
  {
    specs_.reserve( MaxCutSize + 10u );
    incomplete_cut_func_t tmp;
    for ( auto i = 0u; i < MaxCutSize; ++i )
    {
      kitty::create_nth_var( tmp, i );
      specs_.emplace_back( tmp );
    }
  }

  bool run( incomplete_cut_func_t const& func, std::vector<double> times )
  {
    specs_.resize( MaxCutSize );
    std::vector<uint8_t> support( times.size() );
    std::iota( support.begin(), support.end(), 0u );
    auto lit = decompose( support, times, func );
    if ( lit )
      return true;
    return false;
  }

  template<typename Fn>
  bool foreach_spec( Fn&& fn )
  {
    for ( auto i = MaxCutSize; i < specs_.size(); ++i )
    {
      if ( !fn( specs_, i ) )
      {
        return false;
      }
    }
    return true;
  }

private:
  std::optional<uint8_t> decompose( std::vector<uint8_t>& support, std::vector<double>& times, incomplete_cut_func_t func )
  {
// termination condition: the function can be implemented with a gate from the database
minimize_support( support, times, func );
if ( support.size() <= MaxNumVars )
{
  auto lit = static_cast<uint8_t>( specs_.size() );
  specs_.emplace_back( support, func );
  return lit;
}
    return std::nullopt;
  }

  void minimize_support( std::vector<uint8_t>& support, std::vector<double>& times, incomplete_cut_func_t& func )
  {
    auto supp = kitty::min_base_inplace<cut_func_t, true>( func );
    std::vector<uint8_t> new_support( supp.size() );
    std::vector<double> new_times( supp.size() );
    for ( auto i=0u; i < supp.size(); ++i )
    {
      new_support[i] = support[supp[i]];
      new_times[i] = times[supp[i]];
    }
    support = new_support;
    times = new_times;
  }

  /*! \brief Projection functions in the signature space */
  specs_t specs_;
};

} /* namespace synthesis */

} /* namespace mad_hatter */