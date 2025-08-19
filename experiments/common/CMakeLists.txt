# builds a small static lib for common experiment utilities
add_library(rinox_exp_common STATIC
  parser.cpp
)
target_include_directories(rinox_exp_common PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}        # so consumers can include "parser.hpp"
)
target_link_libraries(rinox_exp_common PUBLIC
  rinox_build_options    # provides RINOX_BENCHMARK_DIR define
  rinox_headers          # your public headers / RapidJSON include
)
