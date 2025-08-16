#pragma once

#include "../boolean/support_minimizer.hpp"
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

template<uint32_t MaxCutSize = 6u, uint32_t MaxNumVars = 6u, bool ExacSuppMin = false>
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

    return decompose_( support, times, func ).has_value();
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
  [[nodiscard]] std::optional<uint8_t> decompose_( std::vector<uint8_t> support, std::vector<double> times, incomplete_cut_func_t func )
  {
    supp_minimizer_.run( func, support, times );
    if ( support.size() <= MaxNumVars )
    {
      return termine_decompose_( std::move( support ), std::move( func ) );
    }

    return shannon_decompose_( std::move( support ), std::move( times ), std::move( func ) );
  }

  [[nodiscard]] std::optional<uint8_t>
  termine_decompose_( std::vector<uint8_t> support, incomplete_cut_func_t func )
  {
    const auto lit = static_cast<uint8_t>( specs_.size() );
    specs_.emplace_back( std::move( support ), std::move( func ) ); // now actually moves
    return lit;
  }

  [[nodiscard]] std::optional<uint8_t>
  shannon_decompose_( std::vector<uint8_t> support,
                      std::vector<double> times,
                      incomplete_cut_func_t func )
  {
    auto it = std::max_element( times.begin(), times.end() );
    if ( it == times.end() )
      return std::nullopt;

    size_t index = static_cast<size_t>( std::distance( times.begin(), it ) );
    uint8_t litx = support[index];

    incomplete_cut_func_t func0{ kitty::cofactor0( func._bits, litx ), kitty::cofactor0( func._care, litx ) };
    incomplete_cut_func_t func1{ kitty::cofactor1( func._bits, litx ), kitty::cofactor1( func._care, litx ) };

    // Remove the split variable once for both branches (O(1) with unordered erase)
    auto erase_at_unordered = []( auto& v, size_t idx ) {
      v[idx] = v.back();
      v.pop_back();
    };
    erase_at_unordered( support, index );
    erase_at_unordered( times, index );

    std::vector<uint8_t> supp;
    auto res0 = decompose_( support, times, std::move( func0 ) );
    auto res1 = decompose_( support, times, std::move( func1 ) );
    if ( kitty::is_const0( func0._bits & func0._care ) || kitty::equal( func0._bits & func0._care, func0._care ) )
    {
      if ( res1 )
        supp = { litx, *res1 };
      else
        return std::nullopt;
    }
    else if ( kitty::is_const0( func1._bits & func1._care ) || kitty::equal( func1._bits & func1._care, func1._care ) )
    {
      if ( res0 )
        supp = { litx, *res0 };
      else
        return std::nullopt;
    }
    else
    {
      if ( res0 && res1 )
        supp = { litx, *res0, *res1 };
      else
        return std::nullopt;
    }
    auto lit = static_cast<uint8_t>( specs_.size() );
    specs_.emplace_back( std::move( supp ), func );
    return lit;
  }

private:
  specs_t specs_;
  boolean::support_minimizer<MaxCutSize> supp_minimizer_;
};

} // namespace synthesis

} // namespace rinox
