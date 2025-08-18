#include "parser.hpp"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <stdexcept>
#include <set>

namespace rinox::experiments
{

static inline std::string trim( std::string s )
{
  auto is_space = []( unsigned char c ) { return std::isspace( c ) != 0; };
  s.erase( s.begin(), std::find_if( s.begin(), s.end(), [&]( unsigned char c ) { return !is_space( c ); } ) );
  s.erase( std::find_if( s.rbegin(), s.rend(), [&]( unsigned char c ) { return !is_space( c ); } ).base(), s.end() );
  return s;
}

bool read_text_list_file( const fs::path& path, std::vector<std::string>& out_lines )
{
  std::ifstream in( path );
  if ( !in )
    return false;
  std::string line;
  while ( std::getline( in, line ) )
  {
    line = trim( line );
    if ( line.empty() || line[0] == '#' )
      continue;
    out_lines.push_back( line );
  }
  return true;
}

fs::path default_suite_list_file( const fs::path& suite_dir, const std::string& suite_name )
{
  fs::path p = suite_dir / ( suite_name + ".suite" );
  if ( exists( p ) )
    return p;
  else
  {
    throw std::runtime_error( "File not found" );
  }
}

void normalize_unique( std::vector<std::string>& v )
{
  std::sort( v.begin(), v.end() );
  v.erase( std::unique( v.begin(), v.end() ), v.end() );
}

void collect_from_suite( const fs::path& suite_dir,
                         const std::string& suite_name,
                         const std::string& ext,
                         std::vector<std::string>& bench_names,
                         const std::set<std::string>& exclude_set,
                         std::vector<std::string>& out_files )
{
  if ( !exists( suite_dir ) )
  {
    std::cerr << "[warn] suite dir not found: " << suite_dir << "\n";
    return;
  }

  std::string subdir;
  if ( ext == ".aig" )
    subdir = "aiger";

  std::vector<std::string> listed;
  const fs::path list_path = default_suite_list_file( suite_dir, suite_name );
  if ( read_text_list_file( list_path, listed ) )
  {
    for ( const auto& base : listed )
    {
      bench_names.push_back( base );
      if ( exclude_set.count( base ) )
        continue;
      const fs::path full = suite_dir / subdir / ( base + ext );
      if ( exists( full ) )
        out_files.push_back( full.string() );
      else
        std::cerr << "[warn] listed file missing: " << full << "\n";
    }
  }
  else
  {
    for ( const auto& e : fs::directory_iterator( suite_dir ) )
    {
      if ( !e.is_regular_file() )
        continue;
      if ( e.path().extension() != ext )
        continue;
      const std::string base = e.path().stem().string();
      if ( exclude_set.count( base ) )
        continue;
      out_files.push_back( e.path().string() );
    }
  }
}

static fs::path default_root()
{
  return fs::path( RINOX_BENCHMARK_DIR );
}

std::string normalize_ext( std::string type )
{
  // lowercase
  for ( auto& c : type )
    c = char( std::tolower( unsigned( c ) ) );
  if ( type == "aiger" || type == "aig" )
    return ".aig";
  if ( type == "aag" )
    return ".aag";
  if ( type == "verilog" || type == "v" )
    return ".v";
  if ( type == "blif" )
    return ".blif";
  // fallback: treat as an extension name itself
  if ( !type.empty() && type.front() != '.' )
    type.insert( type.begin(), '.' );
  return type;
}

void load_common_config( const std::string& json_path,
                         BenchSpec& spec_out,
                         TechSpec& tech_spec_out,
                         std::vector<std::string>& files_out )
{
  // --- parse json ---
  FILE* fp = std::fopen( json_path.c_str(), "rb" );
  if ( !fp )
    throw std::runtime_error( "Cannot open config: " + json_path );

  char buf[1 << 16];
  rapidjson::FileReadStream is( fp, buf, sizeof( buf ) );
  rapidjson::Document doc;
  doc.ParseStream( is );
  std::fclose( fp );

  if ( !doc.IsObject() )
    throw std::runtime_error( "Config is not a JSON object: " + json_path );

  // --- benchmarks block ---
  if ( doc.HasMember( "benchmarks" ) && doc["benchmarks"].IsObject() )
  {
    const auto& jb = doc["benchmarks"];

    if ( jb.HasMember( "type" ) && jb["type"].IsString() )
      spec_out.type = jb["type"].GetString();

    if ( jb.HasMember( "root" ) && jb["root"].IsString() )
      spec_out.root = jb["root"].GetString();

    if ( jb.HasMember( "suite" ) && jb["suite"].IsArray() )
      for ( const auto& s : jb["suite"].GetArray() )
        if ( s.IsString() )
          spec_out.suites.emplace_back( s.GetString() );

    if ( jb.HasMember( "names" ) && jb["names"].IsArray() )
      for ( const auto& n : jb["names"].GetArray() )
        if ( n.IsString() )
          spec_out.names.emplace_back( n.GetString() );

    if ( jb.HasMember( "exclude" ) && jb["exclude"].IsArray() )
      for ( const auto& e : jb["exclude"].GetArray() )
        if ( e.IsString() )
          spec_out.exclude.emplace_back( e.GetString() );
  }

  // --- resolve files from suites/names/exclude ---
  files_out.clear();

  normalize_unique( spec_out.suites );
  normalize_unique( spec_out.names );
  normalize_unique( spec_out.exclude );

  const std::string ext = normalize_ext( spec_out.type );
  const std::set<std::string> exclude_set( spec_out.exclude.begin(), spec_out.exclude.end() );

  const fs::path root = spec_out.root.empty() ? default_root() : fs::path( spec_out.root );

  for ( const auto& suite_name : spec_out.suites )
  {
    const fs::path suite_dir = root / suite_name;
    std::cout << suite_dir << std::endl;
    collect_from_suite( suite_dir, suite_name, ext, spec_out.names, exclude_set, files_out );
  }

  // Deterministic order + dedup
  std::sort( files_out.begin(), files_out.end() );
  files_out.erase( std::unique( files_out.begin(), files_out.end() ), files_out.end() );

  if ( doc.HasMember( "techlib" ) && doc["techlib"].IsObject() )
  {
    const auto& jb = doc["techlib"];

    if ( jb.HasMember( "type" ) && jb["type"].IsString() )
      tech_spec_out.type = jb["type"].GetString();

    if ( jb.HasMember( "name" ) && jb["name"].IsString() )
      tech_spec_out.name = jb["name"].GetString();
  }

}

} // namespace rinox::experiments
