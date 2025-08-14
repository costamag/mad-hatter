#pragma once

#include "../dependency/dependency_cut.hpp"
#include <fmt/format.h>
#include <kitty/static_truth_table.hpp>
#include <numeric>
#include <optional>
#include <type_traits>
#include <utility>

namespace rinox
{

namespace synthesis
{

template<uint32_t NumVars = 6u>
struct spec_t
{
  using func_t = kitty::ternary_truth_table<kitty::static_truth_table<NumVars>>;

  std::vector<uint8_t> inputs{};
  func_t sim{};

  spec_t() = default;
  explicit spec_t( const func_t& sim_ ) : sim( sim_ ) {}
  spec_t( std::vector<uint8_t> inputs_, func_t sim_ )
      : inputs( std::move( inputs_ ) ), sim( std::move( sim_ ) ) {}
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

  template<class TimesLike>
  [[nodiscard]] bool run( incomplete_cut_func_t func, const TimesLike& times )
  {
    specs_.resize( MaxCutSize ); // keep the projections, drop previous runs' extras
    std::vector<uint8_t> support;
    support.resize( static_cast<size_t>( times.size() ) );
    std::iota( support.begin(), support.end(), uint8_t{ 0 } );

    return decompose_( support, func ).has_value();
  }

  template<typename Fn>
  bool foreach_spec( Fn&& fn )
  {
    for ( auto i = MaxCutSize; i < specs_.size(); ++i )
    {
      if ( !std::invoke( fn, specs_, i ) )
        return false;
    }
    return true;
  }

private:
  [[nodiscard]] std::optional<uint8_t> decompose_( const std::vector<uint8_t>& support, incomplete_cut_func_t& func )
  {
    auto reduced = reduce_support_( support, func );
    if ( reduced.size() <= MaxNumVars )
    {
      return termine_decompose_( std::move( reduced ), std::move( func ) );
    }
    return std::nullopt;
  }

  [[nodiscard]] std::vector<uint8_t> reduce_support_( const std::vector<uint8_t>& support, incomplete_cut_func_t& func )
  {
    const auto perm = kitty::min_base_inplace<cut_func_t, true>( func );
    std::vector<uint8_t> new_support;
    new_support.reserve( perm.size() );
    for ( auto idx : perm )
      new_support.push_back( support[idx] );
    return new_support;
  }

  [[nodiscard]] std::optional<uint8_t> termine_decompose_( std::vector<uint8_t> support, incomplete_cut_func_t func )
  {
    const auto lit = static_cast<uint8_t>( specs_.size() );
    specs_.emplace_back( std::move( support ), std::move( func ) );
    return lit;
  }

private:
  specs_t specs_;
};

} // namespace synthesis

} // namespace rinox
