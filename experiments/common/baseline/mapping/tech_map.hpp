#pragma once
#include <cstdio>
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <stdexcept>
#include <mockturtle/algorithms/emap.hpp>
#include <mockturtle/utils/tech_library.hpp>

namespace rinox::experiments
{

struct tech_map_t
{
  mockturtle::emap_params mps;
  mockturtle::tech_library_params tps;
};

tech_map_t load_tech_map_params( rapidjson::Document const& doc, lorina::diagnostic_engine * diag );

} // namespace rinox::experiments
