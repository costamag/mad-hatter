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
  \file symmetry.hpp
  \brief Implements utilities to handle functional symmetries

  \author Andrea Costamagna
*/

#pragma once

#include <kitty/kitty.hpp>

namespace rinox
{

namespace boolean
{

  template<typename TT, typename... Containers>
  void min_base_inplace(kitty::ternary_truth_table<TT>& tt,
                        std::vector<uint8_t>& support,
                        Containers&... extra) // lvalue refs
  {
      auto swap_pop_at = [](auto& v, size_t i) {
          if (i + 1 != v.size()) std::swap(v[i], v.back());
          v.pop_back();
      };

      for (size_t i = support.size(); i-- > 0; )
      {
          const auto x = static_cast<uint32_t>(support[i]);
          const auto bits0 = kitty::cofactor0(tt._bits, x);
          const auto care0 = kitty::cofactor0(tt._care, x);
          const auto bits1 = kitty::cofactor1(tt._bits, x);
          const auto care1 = kitty::cofactor1(tt._care, x);
          const auto careA = care1 & care0;

          if (kitty::equal(bits0 & careA, bits1 & careA))
          {
              tt._bits = (bits0 & care0) | (bits1 & care1);
              tt._care = care0 | care1;

              swap_pop_at(support, i);
              (swap_pop_at(extra, i), ...);
          }
      }
  }


} // namespace boolean

} // namespace rinox