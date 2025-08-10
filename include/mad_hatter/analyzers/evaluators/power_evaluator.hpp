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
  \file power_evaluator.hpp
  \brief Analyze the power of a gate-level netlist including glitching

  \author Andrea Costamagna
*/

#pragma once

#include "../../network/signal_map.hpp"
#include "../analyzers_utils/switching.hpp"
#include "../trackers/trackers.hpp"

namespace mad_hatter
{

namespace analyzers
{

namespace evaluators
{

struct power_evaluator_stats
{
  /*! \brief Switching activity */
  double switching = 0;
  /*! \brief Glitching activity */
  double glitching = 0;
  /*! \brief Dynamic power */
  double dyn_power = 0;
};

template<typename Ntk, typename TT, uint32_t TimeSteps = 10>
class power_evaluator
{
public:
  power_evaluator( Ntk& ntk, power_evaluator_stats& st )
      : ntk_( ntk ),
        activity_( ntk ),
        st_( st )
  {
  }

  void run( utils::workload<TT, TimeSteps> const& work )
  {
    st_.glitching = 0;
    st_.switching = 0;
    st_.dyn_power = 0;
    activity_.resize();
    /* store the workload in the input's simulations */
    ntk_.foreach_pi( [&]( auto const& n ) {
      auto const sim = work.get( ntk_.pi_index( n ) );
      activity_[ntk_.make_signal( n )] = sim;
    } );
    double const norm = static_cast<double>( work.num_bits() );

    trackers::arrival_times_tracker arrival( ntk_, work.get_input_arrivals() );
    trackers::sensing_times_tracker sensing( ntk_, work.get_input_sensings() );
    trackers::gate_load_tracker loads( ntk_ );
    trackers::topo_sort_tracker topo_sort( ntk_ );

    std::vector<TT const*> sim_ptrs;
    utils::signal_switching<TT, TimeSteps> tmp_switching;

    /* simulate the network in topological order */
    topo_sort.foreach_gate( [&]( auto const& n ) {
      ntk_.foreach_output( n, [&]( auto const& f ) {
        auto const binding = ntk_.get_binding( f );
        /* simulate the beginning of the clock cycle */
        sim_ptrs.clear();
        ntk_.foreach_fanin( n, [&]( auto const& fi, auto ii ) {
          sim_ptrs.push_back( &activity_[fi][0u] );
        } );
        ntk_.compute( tmp_switching[0u], f, sim_ptrs );

        uint32_t step = 1;
        for ( ; step < TimeSteps; ++step )
          tmp_switching[step] = tmp_switching[0u];

        /* simulate the end of the clock cycle */
        sim_ptrs.clear();
        ntk_.foreach_fanin( n, [&]( auto const& fi, auto ii ) {
          sim_ptrs.push_back( &activity_[fi][TimeSteps - 1] );
        } );
        ntk_.compute( tmp_switching[TimeSteps - 1], f, sim_ptrs );

        step = 1;
        if ( arrival.get_time( f ) > sensing.get_time( f ) )
        {
          while ( step < ( TimeSteps - 1 ) )
          {
            double const time = get_time( step, sensing.get_time( f ) - binding.avg_pin_delay, arrival.get_time( f ) + binding.avg_pin_delay );
            sim_ptrs.clear();
            ntk_.foreach_fanin( n, [&]( auto const& fi, auto ii ) {
              double maxpin = (ii < binding.max_pin_time.size()) ? binding.max_pin_time[ii] : 0.0;

              double const time_i = time - maxpin;
              auto step_i = get_step( time_i, sensing.get_time( fi ) - binding.avg_pin_delay, arrival.get_time( fi ) + binding.avg_pin_delay );
              sim_ptrs.push_back( &activity_[fi][step_i] );
            } );
            ntk_.compute( tmp_switching[step], f, sim_ptrs );

            step++;
          }
        }
        else
        {
          /* just replicate the simulation */
          while ( step++ < TimeSteps / 2 )
            tmp_switching[step] = tmp_switching[0u];
        }
        while ( step++ < ( TimeSteps - 1 ) )
        {
          tmp_switching[step] = tmp_switching[TimeSteps - 1];
        }
        activity_[f] = tmp_switching;
        double glitching = 0;
        double switching = 0;
        double zerodelay = 0;

        for ( int step = 1; step < TimeSteps; ++step )
          switching += kitty::count_ones( activity_[f][step] ^ activity_[f][step - 1] );

        zerodelay = kitty::count_ones( activity_[f][0] ^ activity_[f][TimeSteps - 1] );

        glitching = switching - zerodelay;
        /* normalize */
        glitching /= norm;
        switching /= norm;
        activity_[f].set_glitching( glitching );
        activity_[f].set_switching( switching );
        activity_[f].set_dyn_power( loads.get_load( f ) * switching );
        st_.glitching += activity_[f].get_glitching();
        st_.switching += activity_[f].get_switching();
        st_.dyn_power += activity_[f].get_dyn_power();
      } );
    } );
  }

  void print()
  {
    ntk_.foreach_node( [&]( auto const& n ) {
      ntk_.foreach_output( n, [&]( auto const& f ) {
        std::cout << f.index << " " << f.output << " ";
        for ( int b = 0; b < activity_[f][0].num_bits(); ++b )
        {
          for ( uint32_t step = 0; step < TimeSteps; ++step )
          {
            if ( kitty::get_bit( activity_[f][step], b ) == 1 )
            {
              std::cout << "-";
            }
            else
            {
              std::cout << "_";
            }
          }
          std::cout << " ";
        }
        std::cout << " G:" << activity_[f].get_glitching() << " S:" << activity_[f].get_switching() << " P:" << activity_[f].get_dyn_power() << std::endl;
      } );
    } );
  }

  std::string to_string()
  {
    std::string res = "";
    ntk_.foreach_node( [&]( auto const& n ) {
      ntk_.foreach_output( n, [&]( auto const& f ) {
        res += std::to_string( f.index ) + " " + std::to_string( f.output ) + " ";
        for ( int b = 0; b < activity_[f][0].num_bits(); ++b )
        {
          for ( uint32_t step = 0; step < TimeSteps; ++step )
          {
            if ( kitty::get_bit( activity_[f][step], b ) == 1 )
            {
              res += "-";
            }
            else
            {
              res += "_";
            }
          }
          res += " ";
        }
        res += "\n";
      } );
    } );
    return res;
  }

private:
  /*! \brief get the simulation time given the simulation step in the activity window */
  double get_time( uint32_t step, double const& sensing, double const& arrival ) const
  {
    step = std::max( 0u, std::min( TimeSteps - 1, step ) );
    return sensing + step * ( arrival - sensing ) / ( TimeSteps - 1 );
  }

  /*! \brief get the simulation timestep of a node within the activity window */
uint32_t get_step(double time, double sensing, double arrival) const {
  // Handle degenerate or reversed windows
  if (!(arrival > sensing)) {           // covers equal, reversed, NaN
    return (time <= sensing) ? 0u : (TimeSteps - 1);
  }
  double const denom = arrival - sensing;
  double const t     = (time - sensing) / denom;
  // Clamp before scaling to avoid Inf and huge values
  double const x     = std::clamp(t, 0.0, 1.0);
  double const sf    = (TimeSteps - 1) * x;
  // sf is finite in [0, TimeSteps-1] now
  auto const step    = static_cast<uint32_t>(std::lround(sf));
  return std::min<uint32_t>(TimeSteps - 1, step);
}

private:
  Ntk& ntk_;
  power_evaluator_stats& st_;
  network::incomplete_signal_map<utils::signal_switching<TT, TimeSteps>, Ntk> activity_;
};

} // namespace evaluators

} // namespace analyzers

} // namespace mad_hatter