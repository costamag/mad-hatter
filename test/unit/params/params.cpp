#include <catch2/catch_test_macros.hpp>
#include <iostream>

TEST_CASE("Check compile-time params", "[params]") {
  std::cout << "Params in runtime:\n";
  std::cout << "  RINOX_NUM_VARS_SIGN = " << RINOX_NUM_VARS_SIGN << "\n";
  std::cout << "  RINOX_MAX_CUTS_SIZE = " << RINOX_MAX_CUTS_SIZE << "\n";
  std::cout << "  RINOX_MAX_CUBE_SPFD = " << RINOX_MAX_CUBE_SPFD << "\n";
  std::cout << "  RINOX_MAX_NUM_LEAVES = " << RINOX_MAX_NUM_LEAVES << "\n";
  REQUIRE(RINOX_MAX_NUM_LEAVES > 0);
}