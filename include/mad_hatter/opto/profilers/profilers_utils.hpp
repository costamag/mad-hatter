#pragma once

namespace mad_hatter
{

namespace opto
{

namespace profilers
{

class profiler_params
{
public:
  uint32_t max_num_roots = std::numeric_limits<uint32_t>::max();
  std::vector<double> input_arrivals;
  std::vector<double> output_required;
  double eps = 0.001;
};

} /* namespace profilers */

} /* namespace opto */

} /* namespace mad_hatter */