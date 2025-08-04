/* lorina: C++ parsing library
 * Copyright (C) 2018-2021  EPFL
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
  \file verilog.hpp
  \brief Implements simplistic Verilog parser

  \author Heinz Riener
  \author Mathias Soeken
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include "verilog_reader.hpp"
#include <iostream>
#include <lorina/common.hpp>
#include <lorina/detail/tokenizer.hpp>
#include <lorina/detail/utils.hpp>
#include <lorina/diagnostics.hpp>
#include <lorina/verilog_regex.hpp>
#include <queue>

namespace mad_hatter
{

namespace io
{

namespace verilog
{

class augmented_tokenizer : public lorina::detail::tokenizer
{
public:
  explicit augmented_tokenizer( std::istream& in )
      : lorina::detail::tokenizer( in )
  {}

  lorina::detail::tokenizer_return_code get_token_internal( std::string& token )
  {
    if ( _done )
    {
      return lorina::detail::tokenizer_return_code::invalid;
    }
    token.clear();

    char c;
    while ( get_char( c ) )
    {
      if ( c == '\n' && _comment_mode )
      {
        _comment_mode = false;
        return lorina::detail::tokenizer_return_code::comment;
      }
      else if ( !_comment_mode )
      {
        // Escaped identifier support: starts with '\'
        if ( c == '\\' && !_quote_mode )
        {
          token += c; // keep the backslash in the token
          while ( get_char( c ) )
          {
            // Stop if we hit a delimiter
            if ( c == ' ' || c == '\n' || c == '\t' ||
                 c == ',' || c == ';' || c == ')' || c == '(' )
            {
              // put delimiter back for next token
              lookahead += c;
              break;
            }
            token += c;
          }
          return lorina::detail::tokenizer_return_code::valid;
        }

        if ( ( c == ' ' || c == '\n' ) && !_quote_mode )
        {
          if ( !token.empty() )
            return lorina::detail::tokenizer_return_code::valid;
          else
            continue; // skip leading whitespace
        }

        if ( ( c == '(' || c == ')' || c == '{' || c == '}' ||
               c == ';' || c == ':' || c == ',' || c == '~' ||
               c == '&' || c == '|' || c == '^' || c == '#' ||
               c == '[' || c == ']' ) &&
             !_quote_mode )
        {
          if ( token.empty() )
          {
            token = std::string( 1, c );
          }
          else
          {
            lookahead += c;
          }
          return lorina::detail::tokenizer_return_code::valid;
        }

        if ( c == '"' )
        {
          _quote_mode = !_quote_mode;
        }
      }

      token += c;
    }

    _done = true;
    return lorina::detail::tokenizer_return_code::valid;
  }
};

/*! \brief Simple parser for VERILOG format.
 *
 * Simplistic grammar-oriented parser for a structural VERILOG format.
 *
 */
template<typename verilog_reader>
class verilog_parser
{
public:
  struct module_info
  {
    std::vector<std::string> inputs;
    std::vector<std::string> outputs;
  };

public:
  /*! \brief Construct a VERILOG parser
   *
   * \param in Input stream
   * \param reader A verilog reader
   * \param diag A diagnostic engine
   */
  verilog_parser( std::istream& in,
                  const verilog_reader& reader,
                  lorina::diagnostic_engine* diag = nullptr )
      : tok( in ), reader( reader ), diag( diag ), on_action( PackedFns( GateFn( [&]( const std::vector<std::pair<std::string, bool>>& inputs,
                                                                                      const std::string output,
                                                                                      const std::string type ) {
                                                                           if ( type == "assign" )
                                                                           {
                                                                             assert( inputs.size() == 1u );
                                                                             reader.on_assign( output, inputs[0] );
                                                                           }
                                                                           else if ( type == "and2" )
                                                                           {
                                                                             assert( inputs.size() == 2u );
                                                                             reader.on_and( output, inputs[0], inputs[1] );
                                                                           }
                                                                           else if ( type == "nand2" )
                                                                           {
                                                                             assert( inputs.size() == 2u );
                                                                             reader.on_nand( output, inputs[0], inputs[1] );
                                                                           }
                                                                           else if ( type == "or2" )
                                                                           {
                                                                             assert( inputs.size() == 2u );
                                                                             reader.on_or( output, inputs[0], inputs[1] );
                                                                           }
                                                                           else if ( type == "nor2" )
                                                                           {
                                                                             assert( inputs.size() == 2u );
                                                                             reader.on_nor( output, inputs[0], inputs[1] );
                                                                           }
                                                                           else if ( type == "xor2" )
                                                                           {
                                                                             assert( inputs.size() == 2u );
                                                                             reader.on_xor( output, inputs[0], inputs[1] );
                                                                           }
                                                                           else if ( type == "xnor2" )
                                                                           {
                                                                             assert( inputs.size() == 2u );
                                                                             reader.on_xnor( output, inputs[0], inputs[1] );
                                                                           }
                                                                           else if ( type == "and3" )
                                                                           {
                                                                             assert( inputs.size() == 3u );
                                                                             reader.on_and3( output, inputs[0], inputs[1], inputs[2] );
                                                                           }
                                                                           else if ( type == "or3" )
                                                                           {
                                                                             assert( inputs.size() == 3u );
                                                                             reader.on_or3( output, inputs[0], inputs[1], inputs[2] );
                                                                           }
                                                                           else if ( type == "xor3" )
                                                                           {
                                                                             assert( inputs.size() == 3u );
                                                                             reader.on_xor3( output, inputs[0], inputs[1], inputs[2] );
                                                                           }
                                                                           else if ( type == "maj3" )
                                                                           {
                                                                             assert( inputs.size() == 3u );
                                                                             reader.on_maj3( output, inputs[0], inputs[1], inputs[2] );
                                                                           }
                                                                           else if ( type == "mux21" )
                                                                           {
                                                                             assert( inputs.size() == 3u );
                                                                             reader.on_mux21( output, inputs[0], inputs[1], inputs[2] );
                                                                           }
                                                                           else if ( reader.has_gate( type ) )
                                                                           {
                                                                             std::cout << "I am inside" << std::endl;
                                                                           }
                                                                           else
                                                                           {
                                                                             assert( false && "unknown gate function" );
                                                                             std::cerr << "unknown gate function" << std::endl;
                                                                             std::abort();
                                                                           }
                                                                         } ),
                                                                         ModuleInstFn( [&]( const std::string module_name,
                                                                                            const std::vector<std::string>& params,
                                                                                            const std::string instance_name,
                                                                                            const std::vector<std::pair<std::string, std::string>>& pin_to_pin ) {
                                                                           reader.on_module_instantiation( module_name, params, instance_name, pin_to_pin );
                                                                         } ),
                                                                         CellFn( [&](
                                                                                     const std::vector<std::pair<std::string, std::string>>& input_map,  // input map
                                                                                     const std::vector<std::pair<std::string, std::string>>& output_map, // output map
                                                                                     const std::vector<unsigned int>& ids )                              // some extra string parameter
                                                                                 {
                                                                                   reader.on_cell( input_map, output_map, ids );
                                                                                 } ) ) )
  {
    on_action.declare_known( "0" );
    on_action.declare_known( "1" );
    on_action.declare_known( "1'b0" );
    on_action.declare_known( "1'b1" );
  }

  bool get_token( std::string& token )
  {
    lorina::detail::tokenizer_return_code result;
    do
    {
      if ( tokens.empty() )
      {
        result = tok.get_token_internal( token );
        lorina::detail::trim( token );

        // Normalize escaped identifiers: strip leading backslash
        if ( !token.empty() && token[0] == '\\' )
        {
          token = token.substr( 1 );
        }
      }
      else
      {
        token = tokens.front();
        tokens.pop();

        if ( !token.empty() && token[0] == '\\' )
        {
          token = token.substr( 1 );
        }

        return true;
      }

      /* switch to comment mode */
      if ( token == "//" && result == lorina::detail::tokenizer_return_code::valid )
      {
        tok.set_comment_mode();
      }
      else if ( result == lorina::detail::tokenizer_return_code::comment )
      {
        reader.on_comment( token );
      }
      /* keep parsing if token is empty or if in the middle or at the end of a comment */
    } while ( ( token == "" && result == lorina::detail::tokenizer_return_code::valid ) ||
              tok.get_comment_mode() ||
              result == lorina::detail::tokenizer_return_code::comment );

    return ( result == lorina::detail::tokenizer_return_code::valid );
  }

  void push_token( std::string const& token )
  {
    tokens.push( token );
  }

  bool parse_signal_name()
  {
    valid = get_token( token ); // name or \name
    if ( !valid || token == "[" )
      return false;

    // Handle escaped identifiers like \f[0]
    if ( token[0] == '\\' )
    {
      token = token.substr( 1 );
    }

    auto const name = token;

    valid = get_token( token );
    if ( token == "[" )
    {
      valid = get_token( token ); // size
      if ( !valid )
        return false;
      auto const size = token;

      valid = get_token( token ); // should be "]"
      if ( !valid || token != "]" )
        return false;
      token = name + "[" + size + "]";

      return true;
    }

    push_token( token );
    token = name;
    return true;
  }

  bool parse_modules()
  {
    while ( get_token( token ) )
    {
      if ( token != "module" )
      {
        return false;
      }

      if ( !parse_module() )
      {
        return false;
      }
    }
    return true;
  }

  bool parse_module()
  {
    bool success = parse_module_header();
    if ( !success )
    {
      if ( diag )
      {
        diag->report( lorina::diag_id::ERR_VERILOG_MODULE_HEADER );
      }
      return false;
    }

    do
    {
      valid = get_token( token );
      if ( !valid )
        return false;

      if ( token == "input" )
      {
        success = parse_inputs();
        if ( !success )
        {
          if ( diag )
          {
            diag->report( lorina::diag_id::ERR_VERILOG_INPUT_DECLARATION );
          }
          return false;
        }
      }
      else if ( token == "output" )
      {
        success = parse_outputs();
        if ( !success )
        {
          if ( diag )
          {
            diag->report( lorina::diag_id::ERR_VERILOG_OUTPUT_DECLARATION );
          }
          return false;
        }
      }
      else if ( token == "wire" )
      {
        success = parse_wires();
        if ( !success )
        {
          if ( diag )
          {
            diag->report( lorina::diag_id::ERR_VERILOG_WIRE_DECLARATION );
          }
          return false;
        }
      }
      else if ( token == "parameter" )
      {
        success = parse_parameter();
        if ( !success )
        {
          if ( diag )
          {
            diag->report( lorina::diag_id::ERR_VERILOG_WIRE_DECLARATION );
          }
          return false;
        }
      }
      else
      {
        break;
      }
    } while ( token != "assign" && token != "endmodule" );

    while ( token != "endmodule" )
    {
      if ( token == "assign" )
      {
        success = parse_assign();
        if ( !success )
        {
          if ( diag )
          {
            diag->report( lorina::diag_id::ERR_VERILOG_ASSIGNMENT );
          }
          return false;
        }

        valid = get_token( token );
        if ( !valid )
          return false;
      }
      else if ( reader.has_gate( token ) )
      {
        success = parse_bound_gate();
        if ( !success )
        {
          if ( diag )
          {
            diag->report( lorina::diag_id::ERR_VERILOG_ASSIGNMENT );
          }
          return false;
        }

        valid = get_token( token );
        if ( !valid )
          return false;
      }
      else
      {
        success = parse_module_instantiation();
        if ( !success )
        {
          if ( diag )
          {
            diag->report( lorina::diag_id::ERR_VERILOG_MODULE_INSTANTIATION_STATEMENT );
          }
          return false;
        }

        valid = get_token( token );
        if ( !valid )
          return false;
      }
    }

    /* check dangling objects */
    bool result = true;
    const auto& deps = on_action.unresolved_dependencies();
    if ( deps.size() > 0 )
      result = false;

    for ( const auto& r : deps )
    {
      if ( diag )
      {
        diag->report( lorina::diag_id::WRN_UNRESOLVED_DEPENDENCY )
            .add_argument( r.first )
            .add_argument( r.second );
      }
    }

    if ( !result )
      return false;

    if ( token == "endmodule" )
    {
      /* callback */
      reader.on_endmodule();

      return true;
    }
    else
    {
      return false;
    }
  }

  bool parse_module_header()
  {
    if ( token != "module" )
      return false;

    valid = get_token( token );
    if ( !valid )
      return false;

    module_name = token;

    valid = get_token( token );
    if ( !valid || token != "(" )
      return false;

    std::vector<std::string> inouts;
    do
    {
      if ( !parse_signal_name() )
        return false;
      inouts.emplace_back( token );

      valid = get_token( token ); // , or )
      if ( !valid || ( token != "," && token != ")" ) )
        return false;
    } while ( valid && token != ")" );

    valid = get_token( token );
    if ( !valid || token != ";" )
      return false;

    /* callback */
    reader.on_module_header( module_name, inouts );

    return true;
  }

  bool parse_inputs()
  {
    std::vector<std::string> inputs;
    if ( token != "input" )
      return false;

    std::string size = "";
    if ( !parse_signal_name() || token == "[" )
    {
      do
      {
        valid = get_token( token );
        if ( !valid )
          return false;

        if ( token != "]" )
          size += token;
      } while ( valid && token != "]" );

      if ( !parse_signal_name() )
        return false;
    }
    inputs.emplace_back( token );

    while ( true )
    {
      valid = get_token( token );

      if ( !valid || ( token != "," && token != ";" ) )
        return false;

      if ( token == ";" )
        break;

      if ( !parse_signal_name() )
        return false;

      inputs.emplace_back( token );
    }

    /* callback */
    reader.on_inputs( inputs, size );
    modules[module_name].inputs = inputs;

    for ( const auto& i : inputs )
    {
      on_action.declare_known( i );
    }

    if ( std::smatch m; std::regex_match( size, m, lorina::verilog_regex::const_size_range ) )
    {
      const auto a = std::stoul( m[1].str() );
      const auto b = std::stoul( m[2].str() );
      for ( auto j = std::min( a, b ); j <= std::max( a, b ); ++j )
      {
        for ( const auto& i : inputs )
        {
          on_action.declare_known( fmt::format( "{}[{}]", i, j ) );
        }
      }
    }

    return true;
  }

  bool parse_outputs()
  {
    std::vector<std::string> outputs;
    if ( token != "output" )
      return false;

    std::string size = "";
    if ( !parse_signal_name() || token == "[" )
    {
      do
      {
        valid = get_token( token );
        if ( !valid )
          return false;

        if ( token != "]" )
          size += token;
      } while ( valid && token != "]" );

      if ( !parse_signal_name() )
        return false;
    }
    outputs.emplace_back( token );

    while ( true )
    {
      valid = get_token( token );

      if ( !valid || ( token != "," && token != ";" ) )
        return false;

      if ( token == ";" )
        break;

      if ( !parse_signal_name() )
        return false;

      outputs.emplace_back( token );
    }

    /* callback */
    reader.on_outputs( outputs, size );
    modules[module_name].outputs = outputs;

    return true;
  }

  bool parse_wires()
  {
    std::vector<std::string> wires;
    if ( token != "wire" )
      return false;

    std::string size = "";
    if ( !parse_signal_name() && token == "[" )
    {
      do
      {
        valid = get_token( token );
        if ( !valid )
          return false;

        if ( token != "]" )
          size += token;
      } while ( valid && token != "]" );

      if ( !parse_signal_name() )
        return false;
    }
    wires.emplace_back( token );

    while ( true )
    {
      valid = get_token( token );

      if ( !valid || ( token != "," && token != ";" ) )
        return false;

      if ( token == ";" )
        break;

      if ( !parse_signal_name() )
        return false;

      wires.emplace_back( token );
    }

    /* callback */
    reader.on_wires( wires, size );

    return true;
  }

  bool parse_parameter()
  {
    if ( token != "parameter" )
      return false;

    valid = get_token( token );
    if ( !valid )
      return false;
    auto const name = token;

    valid = get_token( token );
    if ( !valid || ( token != "=" ) )
      return false;

    valid = get_token( token );
    if ( !valid )
      return false;
    auto const value = token;

    valid = get_token( token );
    if ( !valid || ( token != ";" ) )
      return false;

    /* callback */
    reader.on_parameter( name, value );

    return true;
  }

  bool parse_assign()
  {
    if ( token != "assign" )
      return false;

    if ( !parse_signal_name() )
      return false;

    const std::string lhs = token;
    valid = get_token( token );
    if ( !valid || token != "=" )
      return false;

    /* expression */
    bool success = parse_rhs_expression( lhs );
    if ( !success )
    {
      if ( diag )
      {
        diag->report( lorina::diag_id::ERR_VERILOG_ASSIGNMENT_RHS )
            .add_argument( lhs );
      }
      return false;
    }

    if ( token != ";" )
      return false;
    return true;
  }

  bool parse_bound_gate()
  {
    if ( reader.has_gate( token ) == false )
      return false;

    const std::string gate_name = token;

    valid = get_token( token );
    if ( !valid )
      return false;
    const std::string node_name = token;

    /* true for pin name false for signal name */
    std::vector<std::string> inputs;
    std::vector<std::string> outputs;

    valid = get_token( token );
    if ( !valid )
      return false;

    enum class pin_state
    {
      INPUT_PIN,
      OUTPUT_PIN,
      UNKNOWN_PIN
    };
    pin_state state = pin_state::UNKNOWN_PIN;

    std::vector<std::pair<std::string, std::string>> input_assigns;
    std::vector<std::pair<std::string, std::string>> output_assigns;
    auto const ids = reader.get_binding_ids( gate_name );

    while ( token != ";" && token != "endmodule" )
    {
      if ( token == "(" || token == ")" || token == "," )
      {
        valid = get_token( token );
        if ( !valid )
          return false;
        continue;
      }
      /* only signal names and pin names here */
      if ( token[0] == '.' )
      {
        std::string pin_name = token.substr( 1 );
        if ( reader.is_input_pin( gate_name, pin_name ) )
        {
          state = pin_state::INPUT_PIN;
          input_assigns.emplace_back( pin_name, "" );
        }
        else if ( reader.is_output_pin( gate_name, pin_name ) )
        {
          state = pin_state::OUTPUT_PIN;
          output_assigns.emplace_back( pin_name, "" );
        }
        else
        {
          if ( diag )
          {
            diag->report( lorina::diag_id::ERR_VERILOG_ASSIGNMENT )
                .add_argument( token )
                .add_argument( gate_name );
          }
          return false;
        }
      }
      else
      {
        if ( state == pin_state::INPUT_PIN )
        {
          inputs.emplace_back( token );
          input_assigns.back().second = token;
        }
        else if ( state == pin_state::OUTPUT_PIN )
        {
          outputs.emplace_back( token );
          output_assigns.back().second = token;
        }
        else
        {
          if ( diag )
          {
            diag->report( lorina::diag_id::ERR_VERILOG_ASSIGNMENT )
                .add_argument( token )
                .add_argument( gate_name );
          }
          return false;
        }
      }

      valid = get_token( token );
      if ( !valid )
        return false;
    }

    on_action.call_deferred<CELL_FN>( /* dependencies */ inputs, outputs,
                                      /* gate-function params */ std::make_tuple( input_assigns, output_assigns, ids ) );

    return token == ";";
  }

  bool parse_module_instantiation()
  {
    bool success = true;
    std::string const module_name{ token }; // name of module

    auto const it = modules.find( module_name );
    if ( it == std::end( modules ) )
    {
      if ( diag )
      {
        diag->report( lorina::diag_id::ERR_VERILOG_MODULE_INSTANTIATION_UNDECLARED_MODULE )
            .add_argument( module_name );
      }
      return false;
    }

    /* get module info */
    auto const& info = modules[module_name];

    valid = get_token( token );
    if ( !valid )
      return false;

    std::vector<std::string> params;
    if ( token == "#" )
    {
      valid = get_token( token ); // (
      if ( !valid || token != "(" )
        return false;

      do
      {
        valid = get_token( token ); // param
        if ( !valid )
          return false;
        params.emplace_back( token );

        valid = get_token( token ); // ,
        if ( !valid )
          return false;
      } while ( valid && token == "," );

      if ( !valid || token != ")" )
        return false;

      valid = get_token( token );
      if ( !valid )
        return false;
    }

    std::string const inst_name = token; // name of instantiation

    valid = get_token( token );
    if ( !valid || token != "(" )
      return false;

    std::vector<std::pair<std::string, std::string>> args;
    do
    {
      valid = get_token( token );

      if ( !valid )
        return false; // refers to signal
      std::string const arg0{ token };

      /* check if a signal with this name exists in the module declaration */
      if ( ( std::find( std::begin( info.inputs ), std::end( info.inputs ), arg0.substr( 1, arg0.size() ) ) == std::end( info.inputs ) ) &&
           ( std::find( std::begin( info.outputs ), std::end( info.outputs ), arg0.substr( 1, arg0.size() ) ) == std::end( info.outputs ) ) )
      {
        if ( diag )
        {
          diag->report( lorina::diag_id::ERR_VERILOG_MODULE_INSTANTIATION_UNDECLARED_PIN )
              .add_argument( arg0.substr( 1, arg0.size() ) )
              .add_argument( module_name );
        }

        success = false;
      }

      valid = get_token( token );
      if ( !valid || token != "(" )
        return false; // (

      valid = get_token( token );
      if ( !valid )
        return false; // signal name
      auto const arg1 = token;

      valid = get_token( token );
      if ( !valid || token != ")" )
        return false; // )

      valid = get_token( token );
      if ( !valid )
        return false;

      args.emplace_back( std::make_pair( arg0, arg1 ) );
    } while ( token == "," );

    if ( !valid || token != ")" )
      return false;

    valid = get_token( token );
    if ( !valid || token != ";" )
      return false;

    std::vector<std::string> inputs;
    for ( const auto& input : modules[module_name].inputs )
    {
      for ( const auto& a : args )
      {
        if ( a.first.substr( 1, a.first.length() - 1 ) == input )
        {
          inputs.emplace_back( a.second );
        }
      }
    }

    std::vector<std::string> outputs;
    for ( const auto& output : modules[module_name].outputs )
    {
      for ( const auto& a : args )
      {
        if ( a.first.substr( 1, a.first.length() - 1 ) == output )
        {
          outputs.emplace_back( a.second );
        }
      }
    }

    /* callback */
    on_action.call_deferred<MODULE_INST_FN>( inputs, outputs,
                                             std::make_tuple( module_name, params, inst_name, args ) );

    return success;
  }

  bool parse_rhs_expression( const std::string& lhs )
  {
    std::string s;
    do
    {
      valid = get_token( token );
      if ( !valid )
        return false;

      if ( token == ";" || token == "assign" || token == "endmodule" )
        break;
      s.append( token );
    } while ( token != ";" && token != "assign" && token != "endmodule" );

    std::smatch sm;
    if ( std::regex_match( s, sm, lorina::verilog_regex::immediate_assign ) )
    {
      assert( sm.size() == 3u );
      std::vector<std::pair<std::string, bool>> args{ { sm[2], sm[1] == "~" } };

      on_action.call_deferred<GATE_FN>( /* dependencies */ { sm[2] }, { lhs },
                                        /* gate-function params */ std::make_tuple( args, lhs, "assign" ) );
    }
    else if ( std::regex_match( s, sm, lorina::verilog_regex::binary_expression ) )
    {
      assert( sm.size() == 6u );
      std::pair<std::string, bool> arg0 = { sm[2], sm[1] == "~" };
      std::pair<std::string, bool> arg1 = { sm[5], sm[4] == "~" };
      std::vector<std::pair<std::string, bool>> args{ arg0, arg1 };

      auto op = sm[3];

      if ( op == "&" )
      {
        on_action.call_deferred<GATE_FN>( /* dependencies */ { arg0.first, arg1.first }, { lhs },
                                          /* gate-function params */ std::make_tuple( args, lhs, "and2" ) );
      }
      else if ( op == "|" )
      {
        on_action.call_deferred<GATE_FN>( /* dependencies */ { arg0.first, arg1.first }, { lhs },
                                          /* gate-function params */ std::make_tuple( args, lhs, "or2" ) );
      }
      else if ( op == "^" )
      {
        on_action.call_deferred<GATE_FN>( /* dependencies */ { arg0.first, arg1.first }, { lhs },
                                          /* gate-function params */ std::make_tuple( args, lhs, "xor2" ) );
      }
      else
      {
        return false;
      }
    }
    else if ( std::regex_match( s, sm, lorina::verilog_regex::negated_binary_expression ) )
    {
      assert( sm.size() == 6u );
      std::pair<std::string, bool> arg0 = { sm[2], sm[1] == "~" };
      std::pair<std::string, bool> arg1 = { sm[5], sm[4] == "~" };
      std::vector<std::pair<std::string, bool>> args{ arg0, arg1 };

      auto op = sm[3];
      if ( op == "&" )
      {
        on_action.call_deferred<GATE_FN>( /* dependencies */ { arg0.first, arg1.first }, { lhs },
                                          /* gate-function params */ std::make_tuple( args, lhs, "nand2" ) );
      }
      else if ( op == "|" )
      {
        on_action.call_deferred<GATE_FN>( /* dependencies */ { arg0.first, arg1.first }, { lhs },
                                          /* gate-function params */ std::make_tuple( args, lhs, "nor2" ) );
      }
      else if ( op == "^" )
      {
        on_action.call_deferred<GATE_FN>( /* dependencies */ { arg0.first, arg1.first }, { lhs },
                                          /* gate-function params */ std::make_tuple( args, lhs, "xnor2" ) );
      }
      else
      {
        return false;
      }
    }
    else if ( std::regex_match( s, sm, lorina::verilog_regex::ternary_expression ) )
    {
      assert( sm.size() == 9u );
      std::pair<std::string, bool> arg0 = { sm[2], sm[1] == "~" };
      std::pair<std::string, bool> arg1 = { sm[5], sm[4] == "~" };
      std::pair<std::string, bool> arg2 = { sm[8], sm[7] == "~" };
      std::vector<std::pair<std::string, bool>> args{ arg0, arg1, arg2 };

      auto op = sm[3];
      if ( sm[6] != op )
      {
        if ( sm[3] == "?" && sm[6] == ":" )
        {
          on_action.call_deferred<GATE_FN>( /* dependencies */ { arg0.first, arg1.first, arg2.first }, { lhs },
                                            /* gate-function params */ std::make_tuple( args, lhs, "mux21" ) );
          return true;
        }
        return false;
      }

      if ( op == "&" )
      {
        on_action.call_deferred<GATE_FN>( /* dependencies */ { arg0.first, arg1.first, arg2.first }, { lhs },
                                          /* gate-function params */ std::make_tuple( args, lhs, "and3" ) );
      }
      else if ( op == "|" )
      {
        on_action.call_deferred<GATE_FN>( /* dependencies */ { arg0.first, arg1.first, arg2.first }, { lhs },
                                          /* gate-function params */ std::make_tuple( args, lhs, "or3" ) );
      }
      else if ( op == "^" )
      {
        on_action.call_deferred<GATE_FN>( /* dependencies */ { arg0.first, arg1.first, arg2.first }, { lhs },
                                          /* gate-function params */ std::make_tuple( args, lhs, "xor3" ) );
      }
      else
      {
        return false;
      }
    }
    else if ( std::regex_match( s, sm, lorina::verilog_regex::maj3_expression ) )
    {
      assert( sm.size() == 13u );
      std::pair<std::string, bool> a0 = { sm[2], sm[1] == "~" };
      std::pair<std::string, bool> b0 = { sm[4], sm[3] == "~" };
      std::pair<std::string, bool> a1 = { sm[6], sm[5] == "~" };
      std::pair<std::string, bool> c0 = { sm[8], sm[7] == "~" };
      std::pair<std::string, bool> b1 = { sm[10], sm[9] == "~" };
      std::pair<std::string, bool> c1 = { sm[12], sm[11] == "~" };

      if ( a0 != a1 || b0 != b1 || c0 != c1 )
        return false;

      std::vector<std::pair<std::string, bool>> args;
      args.push_back( a0 );
      args.push_back( b0 );
      args.push_back( c0 );

      on_action.call_deferred<GATE_FN>( /* dependencies */ { a0.first, b0.first, c0.first }, { lhs },
                                        /* gate-function params */ std::make_tuple( args, lhs, "maj3" ) );
    }
    else
    {
      return false;
    }

    return true;
  }

private:
  /* Function signatures */
  using GateFn = lorina::detail::Func<
      std::vector<std::pair<std::string, bool>>,
      std::string,
      std::string>;
  using ModuleInstFn = lorina::detail::Func<
      std::string,
      std::vector<std::string>,
      std::string,
      std::vector<std::pair<std::string, std::string>>>;
  using CellFn = lorina::detail::Func<
      std::vector<std::pair<std::string, std::string>>, // input map
      std::vector<std::pair<std::string, std::string>>, // output map
      std::vector<unsigned int>                         // ids
      >;
  using CellParamMap = lorina::detail::ParamPackMap<
      std::string,                                      // key
      std::vector<std::pair<std::string, std::string>>, // input map
      std::vector<std::pair<std::string, std::string>>, // output map
      std::vector<unsigned int>                         // some extra string parameter
      >;
  /* Parameter maps */
  using GateParamMap = lorina::detail::ParamPackMap<
      /* Key */
      std::string,
      /* Params */
      std::vector<std::pair<std::string, bool>>,
      std::string,
      std::string>;
  using ModuleInstParamMap = lorina::detail::ParamPackMap<
      /* Key */
      std::string,
      /* Param */
      std::string,
      std::vector<std::string>,
      std::string,
      std::vector<std::pair<std::string, std::string>>>;

  constexpr static const int GATE_FN{ 0 };
  constexpr static const int MODULE_INST_FN{ 1 };
  constexpr static const int CELL_FN{ 2 };

  using ParamMaps = lorina::detail::ParamPackMapN<GateParamMap, ModuleInstParamMap, CellParamMap>;
  using PackedFns = lorina::detail::FuncPackN<GateFn, ModuleInstFn, CellFn>;

private:
  augmented_tokenizer tok;
  const verilog_reader& reader;
  lorina::diagnostic_engine* diag;

  std::string token;
  std::queue<std::string> tokens; /* lookahead */
  std::string module_name;

  bool valid = false;

  lorina::detail::call_in_topological_order<PackedFns, ParamMaps> on_action;
  std::unordered_map<std::string, module_info> modules;
}; /* verilog_parser */

} // namespace verilog

} // namespace io

} // namespace mad_hatter
