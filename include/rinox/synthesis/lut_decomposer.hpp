#pragma once

#include "../boolean/spfd.hpp"
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

struct lut_decomposer_params
{
  lut_decomposer_params() = default;

  bool try_spfd_decompose = false;
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

  lut_decomposer( lut_decomposer_params ps = {} ) : ps_( ps )
  {
    specs_.reserve( MaxCutSize + 10u );
    incomplete_cut_func_t tmp;
    for ( auto i = 0u; i < MaxCutSize; ++i )
    {
      kitty::create_nth_var( base_[i], i );
      specs_.emplace_back( base_[i] );
    }
  }

  template<class TimesLike>
  [[nodiscard]] bool run( incomplete_cut_func_t func, const TimesLike& times )
  {
    specs_.resize( MaxCutSize );
    std::vector<uint8_t> support( times.size() );
    std::iota( support.begin(), support.end(), 0 ); // Fill with 0, 1, 2, ..., N-1

    std::stable_sort( support.begin(), support.end(), [&]( uint8_t a, uint8_t b ) {
      return times[a] < times[b]; // sort by descending time value
    } );

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
  [[nodiscard]] std::optional<uint8_t> decompose_( std::vector<uint8_t> support, std::vector<double> const& times, incomplete_cut_func_t func )
  {
    supp_minimizer_.run( func, support );
    if ( support.size() <= MaxNumVars )
      return termine_decompose_( std::move( support ), std::move( func ) );

    if ( ps_.try_spfd_decompose )
    {
      auto res = spfd_decompose_( support, times, func );
      if ( res )
        return *res;
    }

    return shannon_decompose_( std::move( support ), times, std::move( func ) );
  }

  [[nodiscard]] std::optional<uint8_t>
  termine_decompose_( std::vector<uint8_t> support, incomplete_cut_func_t func )
  {
    const auto lit = static_cast<uint8_t>( specs_.size() );
    specs_.emplace_back( std::move( support ), std::move( func ) ); // now actually moves
    return lit;
  }

  [[nodiscard]] std::optional<uint8_t>
  spfd_decompose_( std::vector<uint8_t> support, const std::vector<double>& times, incomplete_cut_func_t func )
  {
    spfd_.init( func._bits, func._care );

    const uint32_t index = support.size() - 1;
    const auto varA = base_[index - 1];
    const auto varB = base_[index];

    spfd_.update( varA );
    spfd_.update( varB );

    const uint8_t num_masks = static_cast<uint8_t>( spfd_.get_num_masks() );

    std::vector<uint8_t> alive;
    alive.reserve( 4 );
    for ( uint8_t i = 0; i < num_masks; ++i )
    {
      if ( !spfd_.is_killed( i ) )
        alive.push_back( i );
    }

    switch ( alive.size() )
    {
    case 0:
      return 0;
    case 1:
      return spfd1_decompose_( support, alive, times, index, func );
    case 2:
      return spfd2_decompose_( support, alive, times, index, func );
    case 3:
      return spfd3_decompose_( support, alive, times, index, func );
    case 4:
      return spfd4_decompose_( support, alive, times, index, func );
    default:
      return std::nullopt;
    }
  }

  [[nodiscard]] std::optional<uint8_t>
  try_and_record( incomplete_cut_func_t f,
                  const std::vector<uint8_t>& support,
                  uint32_t index,
                  const std::vector<double>& times,
                  const incomplete_cut_func_t& parent_func )
  {
    std::vector<uint8_t> supp = support;
    supp_minimizer_.run( f, supp );
    if ( supp.size() < support.size() )
    {
      if ( auto res = decompose_( supp, times, f ) )
      {
        specs_.emplace_back( std::vector<uint8_t>{ *res, support[index - 1], support[index] }, parent_func );
        return static_cast<uint8_t>( specs_.size() - 1 );
      }
    }
    return std::nullopt;
  }

  [[nodiscard]] std::optional<uint8_t>
  spfd1_decompose_( std::vector<uint8_t> support, std::vector<uint8_t> alive, std::vector<double> const& times, uint32_t index, incomplete_cut_func_t const& func )
  {
    incomplete_cut_func_t reminder = spfd_.get_function( alive, { '+' } );
    return try_and_record( reminder, support, index, times, func );
  }

  [[nodiscard]] std::optional<uint8_t>
  spfd2_decompose_( std::vector<uint8_t>& support, const std::vector<uint8_t>& alive, const std::vector<double>& times, uint32_t index, incomplete_cut_func_t const& func )
  {
    // Try combined decompositions
    for ( const auto& polarity : { std::vector<char>{ '+', '+' }, std::vector<char>{ '+', '-' } } )
    {
      auto funcr = spfd_.get_function( alive, polarity );
      if ( auto result = try_and_record( funcr, support, index, times, func ); result )
        return result;
    }

    // Try separate decompositions (if allowed)
    if constexpr ( MaxNumVars >= 4u )
    {
      std::array<incomplete_cut_func_t, 2> funcs = {
          spfd_.get_function( { alive[0] }, { '+' } ),
          spfd_.get_function( { alive[1] }, { '+' } ) };

      std::array<std::vector<uint8_t>, 2> supps = { support, support };
      supp_minimizer_.run( funcs[0], supps[0] );
      supp_minimizer_.run( funcs[1], supps[1] );

      if ( supps[0].size() < support.size() && supps[1].size() < support.size() )
      {
        auto res0 = decompose_( supps[0], times, funcs[0] );
        auto res1 = decompose_( supps[1], times, funcs[1] );

        if ( res0 && res1 )
        {
          specs_.emplace_back( std::vector<uint8_t>{ *res0, *res1, support[index - 1], support[index] }, func );
          return static_cast<uint8_t>( specs_.size() - 1 );
        }
      }
    }

    return std::nullopt;
  }

  [[nodiscard]] std::optional<uint8_t>
  spfd3_decompose_( std::vector<uint8_t>& support,
                    const std::vector<uint8_t>& alive,
                    const std::vector<double>& times,
                    uint32_t index, incomplete_cut_func_t const& func )
  {
    // ---- 1) Try combined decompositions (one function of 3 vars) ----
    //   Patterns tried: +++, ++-, +-+, -++
    // Use fixed arrays to avoid heap alloc per iteration.
    static constexpr std::array<std::array<char, 3>, 4> kPol3{ { { { '+', '+', '+' } },
                                                                 { { '+', '+', '-' } },
                                                                 { { '+', '-', '+' } },
                                                                 { { '-', '+', '+' } } } };

    for ( const auto& pol : kPol3 )
    {
      // If your get_function needs std::vector<char>, convert once:
      const std::vector<char> vpol{ pol.begin(), pol.end() };
      auto funcr = spfd_.get_function( alive, vpol );
      if ( auto res = try_and_record( funcr, support, index, times, func ); res )
        return res;
    }

    // ---- 2) Try split into 1-var AND 2-var cases (only if allowed) ----
    if constexpr ( MaxNumVars < 4u )
    {
      return std::nullopt;
    }

    // We will enumerate 6 candidates:
    //   {a}+  with {b,c} in {++ , +-}, and permute a over alive.
    struct OneTwo
    {
      uint8_t a;
      std::array<uint8_t, 2> bc;
      std::array<char, 2> bc_pol; // {++, +-}
    };

    std::array<OneTwo, 6> candidates{ { { alive[0], { { alive[1], alive[2] } }, { { '+', '+' } } },
                                        { alive[0], { { alive[1], alive[2] } }, { { '+', '-' } } },
                                        { alive[1], { { alive[0], alive[2] } }, { { '+', '+' } } },
                                        { alive[1], { { alive[0], alive[2] } }, { { '+', '-' } } },
                                        { alive[2], { { alive[0], alive[1] } }, { { '+', '+' } } },
                                        { alive[2], { { alive[0], alive[1] } }, { { '+', '-' } } } } };

    // Choose the "best" candidate by a cheap cost proxy,
    // while caching the minimized supports to avoid recomputation.
    std::optional<size_t> best_i;
    uint32_t best_cost = ( support.size() - 1 ) * ( support.size() - 1 );

    // We keep these to reuse after picking best candidate (no re-minimize).
    std::vector<uint8_t> best_s0, best_s1;
    incomplete_cut_func_t best_f0, best_f1;

    for ( size_t i = 0; i < candidates.size(); ++i )
    {
      const auto& c = candidates[i];

      // Build functions
      const std::vector<char> pol1{ { '+' } };
      const std::vector<uint8_t> one{ c.a };
      const std::vector<uint8_t> two{ c.bc.begin(), c.bc.end() };
      const std::vector<char> pol2{ c.bc_pol.begin(), c.bc_pol.end() };

      auto f0 = spfd_.get_function( one, pol1 );
      auto f1 = spfd_.get_function( two, pol2 );

      // Minimize both supports once
      std::vector<uint8_t> s0 = support;
      std::vector<uint8_t> s1 = support;
      supp_minimizer_.run( f0, s0 );
      supp_minimizer_.run( f1, s1 );

      const uint32_t cost = static_cast<uint32_t>( s0.size() ) * static_cast<uint32_t>( s1.size() );

      if ( cost <= best_cost )
      {
        best_cost = cost;
        best_i = i;
        best_s0 = std::move( s0 );
        best_s1 = std::move( s1 );
        best_f0 = std::move( f0 );
        best_f1 = std::move( f1 );
      }
    }

    if ( !best_i )
      return std::nullopt;

    // If the chosen pair didn’t reduce either side, bail.
    if ( best_s0.size() >= support.size() || best_s1.size() >= support.size() )
      return std::nullopt;

    // Decompose both; only commit if both succeed.
    auto res0 = decompose_( best_s0, times, best_f0 );
    auto res1 = decompose_( best_s1, times, best_f1 );
    if ( res0 && res1 )
    {
      specs_.emplace_back( std::vector<uint8_t>{ *res0, *res1, support[index - 1], support[index] }, func );
      return static_cast<uint8_t>( specs_.size() - 1 );
    }

    return std::nullopt;
  }

  [[nodiscard]] std::optional<uint8_t>
  spfd4_decompose_( std::vector<uint8_t>& support,
                    const std::vector<uint8_t>& alive,
                    const std::vector<double>& times,
                    uint32_t index, incomplete_cut_func_t const& func )
  {
    // Build function without re-alloc noise. If you can add an overload of
    // spfd_.get_function that takes spans/arrays, use that instead and drop
    // the vector constructions below.
    auto get_fn = [&]( auto const& vars, auto const& pol ) {
      using VarEl = std::decay_t<decltype( vars[0] )>;
      using PolEl = std::decay_t<decltype( pol[0] )>;
      std::vector<VarEl> vv( vars.begin(), vars.end() );
      std::vector<PolEl> pp( pol.begin(), pol.end() );
      return spfd_.get_function( vv, pp );
    };

    // Evaluate a pair of subproblems (fL on varsL/polL, fR on varsR/polR):
    // - minimize both (reusing the minimized supports if this becomes "best"),
    // - compute cost = |suppL| * |suppR|,
    // - update "best" if improved.
    struct Best
    {
      uint32_t cost = std::numeric_limits<uint32_t>::max();
      std::vector<uint8_t> suppL, suppR;
      incomplete_cut_func_t fL, fR;
      bool valid = false;
    };

    auto consider_pair = [&]( auto const& varsL, auto const& polL,
                              auto const& varsR, auto const& polR,
                              Best& best ) {
      auto fL = get_fn( varsL, polL );
      auto fR = get_fn( varsR, polR );

      std::vector<uint8_t> sL = support;
      std::vector<uint8_t> sR = support;

      // Minimize left; if no shrink, we can early prune (can’t beat best)
      supp_minimizer_.run( fL, sL );
      if ( sL.size() >= support.size() )
        return;

      // Minimize right; if no shrink, also prune
      supp_minimizer_.run( fR, sR );
      if ( sR.size() >= support.size() )
        return;

      const uint32_t cost = static_cast<uint32_t>( sL.size() ) * static_cast<uint32_t>( sR.size() );
      if ( cost <= best.cost )
      {
        best.cost = cost;
        best.suppL = std::move( sL );
        best.suppR = std::move( sR );
        best.fL = std::move( fL );
        best.fR = std::move( fR );
        best.valid = true;
      }
    };

    // If your logic wants all 1-negative patterns first (weight 3), list them:
    // (Adjust this set to the exact polarity families you want to test.)
    static constexpr std::array<std::array<char, 4>, 4> kPol4_oneMinus{ { { { '-', '+', '+', '+' } },
                                                                          { { '+', '-', '+', '+' } },
                                                                          { { '+', '+', '-', '+' } },
                                                                          { { '+', '+', '+', '-' } } } };

    for ( auto const& pol : kPol4_oneMinus )
    {
      auto f = get_fn( alive, pol );
      if ( auto r = try_and_record( f, support, index, times, func ); r )
        return r;
    }

    // Optionally also try two-minus patterns; include if it helps your search:
    static constexpr std::array<std::array<char, 4>, 6> kPol4_twoMinus{ { { { '-', '-', '+', '+' } },
                                                                          { { '-', '+', '-', '+' } },
                                                                          { { '-', '+', '+', '-' } },
                                                                          { { '+', '-', '-', '+' } },
                                                                          { { '+', '-', '+', '-' } },
                                                                          { { '+', '+', '-', '-' } } } };
    for ( auto const& pol : kPol4_twoMinus )
    {
      auto f = get_fn( alive, pol );
      if ( auto r = try_and_record( f, support, index, times, func ); r )
        return r;
    }

    if constexpr ( MaxNumVars < 4u )
    {
      return std::nullopt;
    }

    // 2a) 1 + 3 split: choose which var is alone (+), the other three get a polarity triple.
    // We evaluate all 4*4 = 16 candidates and keep the best by cost.
    Best best13;

    static constexpr std::array<std::array<char, 1>, 1> kPol1{ { { { '+' } } } };
    static constexpr std::array<std::array<char, 3>, 4> kPol3{ { { { '+', '+', '+' } },
                                                                 { { '+', '+', '-' } },
                                                                 { { '+', '-', '+' } },
                                                                 { { '-', '+', '+' } } } };

    for ( int a = 0; a < 4; ++a )
    {
      std::array<uint8_t, 1> one{ { alive[a] } };
      std::array<uint8_t, 3> three{};
      {
        int t = 0;
        for ( int i = 0; i < 4; ++i )
          if ( i != a )
            three[t++] = alive[i];
      }
      for ( auto const& p3 : kPol3 )
        consider_pair( one, kPol1[0], three, p3, best13 );
    }

    if ( best13.valid )
    {
      auto res0 = decompose_( best13.suppL, times, best13.fL );
      auto res1 = decompose_( best13.suppR, times, best13.fR );
      if ( res0 && res1 )
      {
        specs_.emplace_back( std::vector<uint8_t>{ *res0, *res1, support[index - 1], support[index] }, func );
        return static_cast<uint8_t>( specs_.size() - 1 );
      }
    }

    // 2b) 2 + 2 split: three pairings (AB|CD, AC|BD, AD|BC) × 4 polarity pairs
    Best best22;

    static constexpr std::array<std::array<char, 2>, 4> kPol2{ { { { '+', '+' } },
                                                                 { { '+', '-' } },
                                                                 { { '-', '+' } },
                                                                 { { '-', '-' } } } };

    // All 3 perfect matchings of 4 items:
    std::array<std::array<uint8_t, 2>, 3> L{ { { { alive[0], alive[1] } },
                                               { { alive[0], alive[2] } },
                                               { { alive[0], alive[3] } } } };
    std::array<std::array<uint8_t, 2>, 3> R{ { { { alive[2], alive[3] } },
                                               { { alive[1], alive[3] } },
                                               { { alive[1], alive[2] } } } };

    for ( int m = 0; m < 3; ++m )
    {
      for ( auto const& pL : kPol2 )
        for ( auto const& pR : kPol2 )
          consider_pair( L[m], pL, R[m], pR, best22 );
    }

    if ( best22.valid )
    {
      auto res0 = decompose_( best22.suppL, times, best22.fL );
      auto res1 = decompose_( best22.suppR, times, best22.fR );
      if ( res0 && res1 )
      {
        specs_.emplace_back( std::vector<uint8_t>{ *res0, *res1, support[index - 1], support[index] }, func );
        return static_cast<uint8_t>( specs_.size() - 1 );
      }
    }

    return std::nullopt;
  }

  [[nodiscard]] std::optional<uint8_t>
  shannon_decompose_( std::vector<uint8_t> support,
                      std::vector<double> const& times,
                      incomplete_cut_func_t func )
  {

    auto litx = support.back();

    incomplete_cut_func_t func0{ kitty::cofactor0( func._bits, litx ), kitty::cofactor0( func._care, litx ) };
    incomplete_cut_func_t func1{ kitty::cofactor1( func._bits, litx ), kitty::cofactor1( func._care, litx ) };

    support.pop_back();
    auto res0 = decompose_( support, times, std::move( func0 ) );
    auto res1 = decompose_( support, times, std::move( func1 ) );

    std::vector<uint8_t> supp;
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
  lut_decomposer_params ps_;
  specs_t specs_;
  std::array<cut_func_t, MaxCutSize> base_;
  boolean::support_minimizer<MaxCutSize> supp_minimizer_;
  boolean::spfd<cut_func_t, 1u << MaxCutSize> spfd_;
};

} // namespace synthesis

} // namespace rinox
