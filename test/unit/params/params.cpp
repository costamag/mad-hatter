#include <catch2/catch_test_macros.hpp>
#include <iostream>

TEST_CASE("Check compile-time params", "[params]") {
  std::cout << "Params in runtime:\n";
  std::cout << "  HATTER_NUM_VARS_SIGN = " << HATTER_NUM_VARS_SIGN << "\n";
  std::cout << "  HATTER_MAX_CUTS_SIZE = " << HATTER_MAX_CUTS_SIZE << "\n";
  std::cout << "  HATTER_MAX_CUBE_SPFD = " << HATTER_MAX_CUBE_SPFD << "\n";
  std::cout << "  HATTER_MAX_NUM_LEAVES = " << HATTER_MAX_NUM_LEAVES << "\n";
  REQUIRE(HATTER_MAX_NUM_LEAVES > 0);
}