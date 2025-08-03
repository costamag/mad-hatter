/* mad_hatter: C++ logic network library
 * Copyright (C) 2018-2022  EPFL
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
  \file workload.hpp
  \brief Utilities to represent simulation patterns for power simulation

  \author Andrea Costamagna
*/

#pragma once

namespace mad_hatter
{

namespace power
{

template<typename TT, uint32_t TimeSteps>
class signal_switching
{
public:
  signal_switching() = default;
  /*! \brief */
  signal_switching( TT const& tt_init, TT const& tt_end )
  {
    int step = 0;
    while ( step < TimeSteps / 2 )
    {
      sims_[step++] = tt_init;
    }
    while ( step < TimeSteps )
    {
      sims_[step++] = tt_end;
    }
  }

  uint32_t num_bits() const
  {
    return sims_[0].num_bits();
  }

  void reset()
  {
    switching_ = 0;
    glitching_ = 0;
    dyn_power_ = 0;
  }

  void set_switching( double const& switching )
  {
    switching_ = switching;
  }

  void set_glitching( double const& glitching )
  {
    glitching_ = glitching;
  }

  void set_dyn_power( double const& dyn_power )
  {
    dyn_power_ = dyn_power;
  }

  double get_switching() const
  {
    return switching_;
  }

  double get_glitching() const
  {
    return glitching_;
  }

  double get_dyn_power() const
  {
    return dyn_power_;
  }

  TT& operator[]( uint32_t const& step )
  {
    return sims_[step];
  }

  signal_switching<TT, TimeSteps> const& operator[]( uint32_t const& step ) const
  {
    return sims_[step];
  }

private:
  std::array<TT, TimeSteps> sims_;
  double switching_ = 0;
  double glitching_ = 0;
  double dyn_power_ = 0;
};

template<typename TT, uint32_t TimeSteps>
class workload
{
public:
  workload( std::vector<TT> const& tts_init, std::vector<TT> const& tts_end )
      : num_inputs_( tts_init.size() )
  {
    for ( int i = 0; i < num_inputs_; ++i )
    {
      sims_.emplace_back( tts_init[i], tts_end[i] );
    }
  }

public:
  std::vector<double> const& get_input_arrivals() const
  {
    return arrival_;
  }

  std::vector<double> const& get_input_sensings() const
  {
    return sensing_;
  }

  signal_switching<TT, TimeSteps> const& get( uint32_t index ) const
  {
    return sims_[index];
  }

  uint32_t num_bits() const
  {
    return sims_[0].num_bits();
  }

  signal_switching<TT, TimeSteps>& operator[]( uint32_t const& step )
  {
    return sims_[step];
  }

  signal_switching<TT, TimeSteps> const& operator[]( uint32_t const& step ) const
  {
    return sims_[step];
  }

private:
  uint32_t num_inputs_;
  /* the simulations at the beginning of the clock cycle */
  std::vector<signal_switching<TT, TimeSteps>> sims_;
  /* the arrival  time at the inputs */
  std::vector<double> arrival_;
  /* the sensing time at the inputs */
  std::vector<double> sensing_;
};

} // namespace power

} // namespace mad_hatter