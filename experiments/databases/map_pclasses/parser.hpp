// experiments/opto/resynthesis/parser.hpp
#pragma once
#include "../../common/parser.hpp" // load_common_config(...)
#include <cstdio>
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <stdexcept>

namespace rinox
{

namespace experiments
{

template<typename Params>
inline void load_database_generation_params_from_value(const rapidjson::Value& root, Params& ps) {
  if (!root.IsObject()) return;

  if (!root.HasMember("parameters") || !root["parameters"].IsObject()) return;
  const auto& tm = root["parameters"];

  if (tm.HasMember("num_vars") && tm["num_vars"].IsUint())
    ps.num_vars = tm["num_vars"].GetUint();

  if (tm.HasMember("multiple_candidates") && tm["multiple_candidates"].IsBool())
    ps.multiple_candidates = tm["multiple_candidates"].GetBool();

  if (tm.HasMember("verbose") && tm["verbose"].IsBool())
    ps.verbose = tm["verbose"].GetBool();

  if (tm.HasMember("metric") && tm["metric"].IsString())
    ps.metric = tm["metric"].GetString();
}

template<typename Params>
inline void load_database_generation_params(const std::string& json_path, Params& ps )
{
  FILE* fp = std::fopen(json_path.c_str(), "rb");
  if (!fp) throw std::runtime_error("Cannot open config: " + json_path);

  char buf[1 << 16];
  rapidjson::FileReadStream is(fp, buf, sizeof(buf));
  rapidjson::Document doc;
  doc.ParseStream(is);
  std::fclose(fp);

  if (!doc.IsObject())
    throw std::runtime_error("Config is not a JSON object: " + json_path);

  load_database_generation_params_from_value(doc, ps);
}

template<class Params>
inline void load_config( const std::string& path,
                         Params& ps,
                         rinox::experiments::BenchSpec& spec,
                         rinox::experiments::TechSpec& tech )
{
  std::vector<std::string> files;
  rinox::experiments::load_common_config( path, spec, tech, files );
  load_database_generation_params( path, ps );
}

} // namespace experiments

} // namespace rinox
