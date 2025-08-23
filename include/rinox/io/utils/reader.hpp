/* mockturtle: C++ logic network library
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
  \file reader.hpp
  \brief Lorina reader for VERILOG files

  \author Andrea Costamagna
*/

#pragma once

#include <algorithm>
#include <cctype>
#include <iostream>
#include <map>
#include <regex>
#include <string>
#include <vector>

#include <fmt/format.h>
#include <lorina/verilog.hpp>
#include <mockturtle/generators/modular_arithmetic.hpp>

#include "../../network/network.hpp"
#include "../../traits.hpp"

namespace rinox
{

namespace io
{

/*! \brief Lorina reader callback for VERILOG files.
 *
 * **Required network functions:**
 * - `create_pi`
 * - `create_po`
 * - `get_constant`
 * - `create_not`
 * - `create_and`
 * - `create_or`
 * - `create_xor`
 * - `create_maj`
 *
   \verbatim embed:rst

   Example

   .. code-block:: c++

      mig_network mig;
      lorina::read_verilog( "file.v", reader( mig ) );
   \endverbatim
 */
template<typename Ntk>
class reader : public lorina::verilog_reader
{
public:
  explicit reader( Ntk& ntk, std::string const& top_module_name = "top" ) : ntk_( ntk ), top_module_name_( top_module_name )
  {
    static_assert( mockturtle::is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( mockturtle::has_create_pi_v<Ntk>, "Ntk does not implement the create_pi function" );
    static_assert( mockturtle::has_create_po_v<Ntk>, "Ntk does not implement the create_po function" );
    static_assert( mockturtle::has_get_constant_v<Ntk>, "Ntk does not implement the get_constant function" );
    static_assert( mockturtle::has_create_not_v<Ntk>, "Ntk does not implement the create_not function" );
    static_assert( mockturtle::has_create_and_v<Ntk>, "Ntk does not implement the create_and function" );
    static_assert( mockturtle::has_create_or_v<Ntk>, "Ntk does not implement the create_or function" );
    static_assert( mockturtle::has_create_xor_v<Ntk>, "Ntk does not implement the create_xor function" );
    static_assert( mockturtle::has_create_ite_v<Ntk>, "Ntk does not implement the create_ite function" );
    static_assert( mockturtle::has_create_maj_v<Ntk>, "Ntk does not implement the create_maj function" );

    signals_["0"] = ntk_.get_constant( false );
    signals_["1"] = ntk_.get_constant( true );
    std::vector<std::string> bases{ "h", "b", "d", "o" };
    for ( auto const& base : bases )
    {
      signals_["1'" + base + "0"] = ntk_.get_constant( false );
      signals_["1'" + base + "1"] = ntk_.get_constant( true );
    }

    if constexpr ( mockturtle::has_set_name_v<Ntk> )
    {
      ntk_.set_name( signals_["0"], "1'b0" );
      ntk_.set_name( signals_["1"], "1'b1" );
    }
  }

  void on_module_header( const std::string& module_name, const std::vector<std::string>& inouts ) const override
  {
    (void)inouts;
    if constexpr ( mockturtle::has_set_network_name_v<Ntk> )
    {
      ntk_.set_network_name( module_name );
    }

    name_ = module_name;
  }

  void on_inputs( const std::vector<std::string>& names, std::string const& size = "" ) const override
  {
    // if ( name_ != top_module_name_ )
    //   return;

    for ( const auto& name : names )
    {
      if ( size.empty() )
      {
        signals_[name] = ntk_.create_pi();
        input_names_.emplace_back( name );
        if constexpr ( mockturtle::has_set_name_v<Ntk> )
        {
          ntk_.set_name( signals_[name], name );
        }
      }
      else
      {
        std::vector<typename Ntk::signal> word;
        const auto length = parse_size( size );
        for ( auto i = 0u; i < length; ++i )
        {
          const auto sname = length > 1 ? fmt::format( "{}[{}]", name, i ) : fmt::format( "{}", name );
          word.push_back( ntk_.create_pi() );
          signals_[sname] = word.back();
          if constexpr ( mockturtle::has_set_name_v<Ntk> )
          {
            ntk_.set_name( signals_[sname], sname );
          }
          input_names_.emplace_back( sname );
        }
        registers_[name] = word;
      }
    }
  }

  void on_input( std::string const& name, std::vector<std::string> const& ids, bool upto = false ) const
  {
    // if ( name_ != top_module_name_ )
    //   return;

    int first = upto ? 0 : ids.size() - 1;
    int last = upto ? ids.size() - 1 : 0;
    int i = first;
    int di = upto ? +1 : -1;
    std::vector<typename Ntk::signal> word;
    while ( i != ( last + di ) )
    {
      const auto& id = ids[i];
      const auto sname = ids.size() > 1 ? fmt::format( "{}[{}]", name, i ) : fmt::format( "{}", name );
      word.push_back( ntk_.create_pi() );
      signals_[id] = word.back();
      if constexpr ( mockturtle::has_set_name_v<Ntk> )
      {
        ntk_.set_name( signals_[id], sname );
      }
      registers_[name] = word;
      input_names_.emplace_back( sname );
      i += di;
    }
  }

  void on_outputs( const std::vector<std::string>& names, std::string const& size = "" ) const override
  {
    (void)size;
    // if ( name_ != top_module_name_ )
    //   return;

    for ( const auto& name : names )
    {
      if ( size.empty() )
      {
        outputs_.emplace_back( name );
        output_names_.emplace_back( name );
      }
      else
      {
        const auto length = parse_size( size );
        for ( auto i = 0u; i < length; ++i )
        {
          outputs_.emplace_back( fmt::format( "{}[{}]", name, i ) );
        }
        output_names_.emplace_back( outputs_.back() );
      }
    }
  }

  void on_output( const std::string name, const std::vector<std::string>& ids, const bool upto = false ) const
  {
    // if ( name_ != top_module_name_ )
    //   return;

    int first = upto ? 0 : ids.size() - 1;
    int last = upto ? ids.size() - 1 : 0;
    int i = first;
    int di = upto ? +1 : -1;
    while ( i != ( last + di ) )
    {
      const auto& id = ids[i];
      const auto sname = ids.size() > 1 ? fmt::format( "{}[{}]", name, i ) : fmt::format( "{}", name );
      output_names_.emplace_back( sname );
      outputs_.emplace_back( id );
      i += di;
    }
  }

  void on_wires( const std::vector<std::string>& names, std::string const& size = "" ) const override
  {
  }

  void on_assign( const std::string& lhs, const std::pair<std::string, bool>& rhs ) const override
  {
    // if ( name_ != top_module_name_ )
    //   return;

    if ( signals_.find( rhs.first ) == signals_.end() )
      fmt::print( stderr, "[w] undefined signal {} assigned 0\n", rhs.first );

    auto r = signals_[rhs.first];
    signals_[lhs] = rhs.second ? ntk_.create_not( r ) : r;
  }

  void on_cell( const std::vector<std::pair<std::string, std::string>>& input_assign,
                const std::vector<std::pair<std::string, std::string>>& output_assign,
                const std::vector<unsigned int>& ids ) const
  {
    if constexpr ( traits::is_bound_network_type_v<Ntk> )
    {
      // if ( name_ != top_module_name_ )
      //   return;

      if ( signals_.find( output_assign[0].second ) != signals_.end() )
      {
        return;
      }
      std::vector<typename Ntk::signal> children( input_assign.size() );
      for ( auto const& child : input_assign )
      {
        if ( signals_.find( child.second ) == signals_.end() )
          fmt::print( stderr, "[w] undefined signal {} assigned 0\n", child.second );
        auto const j = ntk_.get_fanin_number( ids[0], child.first );
        children[j] = signals_[child.second];
      }

      auto signal = ntk_.create_node( children, ids );
      for ( auto i = 0u; i < output_assign.size(); ++i )
      {
        signals_[output_assign[i].second] = { signal.index, i };
      }
    }
    else
    {
      fmt::print( stderr, "[e] this network type is not supported\n" );
    }
  }

  void on_module_instantiation( std::string const& module_name, std::vector<std::string> const& params, std::string const& inst_name,
                                std::vector<std::pair<std::string, std::string>> const& args ) const override
  {
    (void)inst_name;
    // if ( name_ != top_module_name_ )
    //   return;

    /* check routines */
    const auto num_args_equals = [&]( uint32_t expected_count ) {
      if ( args.size() != expected_count )
      {
        fmt::print( stderr, "[e] {} module expects {} arguments\n", module_name, expected_count );
        return false;
      }
      return true;
    };

    const auto num_params_equals = [&]( uint32_t expected_count ) {
      if ( params.size() != expected_count )
      {
        fmt::print( stderr, "[e] {} module expects {} parameters\n", module_name, expected_count );
        return false;
      }
      return true;
    };

    const auto register_exists = [&]( std::string const& name ) {
      if ( registers_.find( name ) == registers_.end() )
      {
        fmt::print( stderr, "[e] register {} does not exist\n", name );
        return false;
      }
      return true;
    };

    const auto register_has_size = [&]( std::string const& name, uint32_t size ) {
      if ( !register_exists( name ) || registers_[name].size() != size )
      {
        fmt::print( stderr, "[e] register {} must have size {}\n", name, size );
        return false;
      }
      return true;
    };

    const auto add_register = [&]( std::string const& name, std::vector<typename Ntk::signal> const& fs ) {
      for ( auto i = 0u; i < fs.size(); ++i )
      {
        signals_[fmt::format( "{}[{}]", name, i )] = fs[i];
      }
      registers_[name] = fs;
    };

    fmt::print( stderr, "[e] unknown module name {}\n", module_name );
  }

  void on_endmodule() const override
  {
    // if ( name_ != top_module_name_ )
    //   return;

    for ( auto const& o : outputs_ )
    {
      ntk_.create_po( signals_[o] );
    }

    if constexpr ( mockturtle::has_set_output_name_v<Ntk> )
    {
      uint32_t ctr{ 0u };
      for ( auto const& output_name : output_names_ )
      {
        ntk_.set_output_name( ctr++, output_name );
      }
      assert( ctr == ntk_.num_pos() );
    }
  }

  const std::string& name() const
  {
    return name_;
  }

  const std::vector<std::string> input_names()
  {
    return input_names_;
  }

  const std::vector<std::string> output_names()
  {
    return output_names_;
  }

  bool has_gate( std::string const& name ) const
  {
    if constexpr ( traits::is_bound_network_type_v<Ntk> )
      return ntk_.has_gate( name );
    return false;
  }

  std::vector<unsigned int> get_binding_ids( std::string const& name ) const
  {
    if constexpr ( traits::is_bound_network_type_v<Ntk> )
      return ntk_.get_binding_ids( name );
    std::cerr << "[e] Ntk does not support get_binding_ids\n";
    return {};
  }

  bool is_input_pin( std::string const& gate_name, std::string const& pin_name ) const
  {
    return ntk_.is_input_pin( gate_name, pin_name );
  }

  bool is_output_pin( std::string const& gate_name, std::string const& pin_name ) const
  {
    return ntk_.is_output_pin( gate_name, pin_name );
  }

  unsigned int get_pin_id( std::string const& gate_name, std::string const& pin_name ) const
  {
    return ntk_.get_pin_id( gate_name, pin_name );
  }

  bool ends_with_index_zero( const std::string& s ) const
  {
    return s.size() >= 3 && s.substr( s.size() - 3 ) == "[0]";
  }

  bool ends_with_index( const std::string& s ) const
  {
    if ( s.empty() || s.back() != ']' )
      return false;

    size_t open_bracket = s.rfind( '[' );
    if ( open_bracket == std::string::npos || open_bracket > s.size() - 2 )
      return false;

    // Check if all characters between [ and ] are digits
    for ( size_t i = open_bracket + 1; i < s.size() - 1; ++i )
    {
      if ( !std::isdigit( s[i] ) )
        return false;
    }

    return true;
  }

  std::string remove_index( const std::string& s ) const
  {
    size_t open_bracket = s.rfind( '[' );
    if ( open_bracket != std::string::npos && s.back() == ']' )
    {
      return s.substr( 0, open_bracket );
    }
    return s;
  }

  void sanitize_input_names() const
  {
    std::unordered_map<std::string, std::pair<uint32_t, uint32_t>> map;
    ntk_.foreach_pi( [&]( auto const& f, auto i ) {
      std::string name = ntk_.get_input_name( i );
      if ( ends_with_index( name ) )
      {
        std::string trimmed = remove_index( name );
        auto it = map.find( trimmed );
        if ( it != map.end() )
        {
          map[trimmed].second++;
        }
        else
        {
          map[trimmed] = std::make_pair( i, 1 );
        }
      }
    } );
    ntk_.foreach_pi( [&]( auto const& f, auto i ) {
      std::string name = ntk_.get_input_name( i );
      std::string trimmed = remove_index( name );
      if ( map[trimmed].second == 1 )
      {
        if ( ends_with_index_zero( name ) )
        {
          std::string trimmed = name.substr( 0, name.size() - 3 );
          ntk_.set_input_name( i, trimmed );
        }
      }
    } );
  }

  void sanitize_output_names() const
  {
    std::unordered_map<std::string, std::pair<uint32_t, uint32_t>> map;
    ntk_.foreach_po( [&]( auto const& f, auto i ) {
      std::string name = ntk_.get_output_name( i );
      auto it = map.find( name );
      if ( it != map.end() )
      {
        map[name].second++;
      }
      else
      {
        map[name] = std::make_pair( i, 1 );
      }
    } );
    ntk_.foreach_po( [&]( auto const& f, auto i ) {
      std::string name = ntk_.get_output_name( i );
      if ( map[name].second == 1 )
      {
        if ( ends_with_index_zero( name ) )
        {
          std::string trimmed = name.substr( 0, name.size() - 3 );
          ntk_.set_output_name( i, trimmed );
        }
      }
    } );
  }

private:
  std::vector<bool> parse_value( const std::string& value ) const
  {
    std::smatch match;

    if ( std::all_of( value.begin(), value.end(), isdigit ) )
    {
      std::vector<bool> res( 64u );
      mockturtle::bool_vector_from_dec( res, static_cast<uint64_t>( std::stoul( value ) ) );
      return res;
    }
    else if ( std::regex_match( value, match, hex_string ) )
    {
      std::vector<bool> res( static_cast<uint64_t>( std::stoul( match.str( 1 ) ) ) );
      mockturtle::bool_vector_from_hex( res, match.str( 2 ) );
      return res;
    }
    else
    {
      fmt::print( stderr, "[e] cannot parse number '{}'\n", value );
    }
    assert( false );
    return {};
  }

  uint64_t parse_small_value( const std::string& value ) const
  {
    return mockturtle::bool_vector_to_long( parse_value( value ) );
  }

  uint32_t parse_size( const std::string& size ) const
  {
    if ( size.empty() )
    {
      return 1u;
    }

    if ( auto const l = size.size(); l > 2 && size[l - 2] == ':' && size[l - 1] == '0' )
    {
      return static_cast<uint32_t>( parse_small_value( size.substr( 0u, l - 2 ) ) + 1u );
    }

    assert( false );
    return 0u;
  }

private:
  Ntk& ntk_;

  std::string const top_module_name_;

  mutable std::map<std::string, typename Ntk::signal> signals_;
  mutable std::map<std::string, std::vector<typename Ntk::signal>> registers_;
  mutable std::vector<std::string> outputs_;
  mutable std::string name_;
  mutable std::vector<std::string> input_names_;
  mutable std::vector<std::string> output_names_;

  std::regex hex_string{ "(\\d+)'h([0-9a-fA-F]+)" };
};

} /* namespace io */

} // namespace rinox