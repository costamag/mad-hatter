cmake_minimum_required(VERSION 3.16)
project(mad_hatter)

# Project-specific options
option(HATTER_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(HATTER_ENABLE_TESTS "Build unit tests" ON)

# Hide dependency options from ccmake by default
mark_as_advanced(FORCE
  MOCKTURTLE_BUILD_EXAMPLES
  MOCKTURTLE_BUILD_EXPERIMENTS
  MOCKTURTLE_ENABLE_COVERAGE
  MOCKTURTLE_ENABLE_ASAN
  FETCHCONTENT_BASE_DIR
  FETCHCONTENT_FULLY_DISCONNECTE
  FETCHCONTENT_QUIET
  FETCHCONTENT_SOURCE_DIR_CATCH2
  FETCHCONTENT_UPDATES_DISCONNEC
  CATCH_DEVELOPMENT_BUILD
  CATCH_INSTALL_DOCS
  CATCH_INSTALL_EXTRAS
  FETCHCONTENT_FULLY_DISCONNECTE
  FETCHCONTENT_UPDATES_DISCONNEC
)

if (HATTER_ENABLE_ASAN)
  set(SANITIZE_FLAGS "-fsanitize=address -fno-omit-frame-pointer")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZE_FLAGS}")
  set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} ${SANITIZE_FLAGS}")
endif()

set(CMAKE_CXX_STANDARD 17)

# Enable AVX2 if available
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  message(STATUS "[mad_hatter] Enabling AVX2 for SIMD operations")
  add_compile_options(-mavx2 -mfma)
elseif (MSVC)
  add_compile_options(/arch:AVX2)
endif()

# Dependencies
add_subdirectory(lib/mockturtle)
add_subdirectory(lib/replxx)

# CLI executable
add_executable(hatter src/main.cpp)
target_link_libraries(hatter PRIVATE mockturtle replxx)

# Library
add_library(mad_hatter INTERFACE)
target_include_directories(mad_hatter INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Enable tests if requested
if (HATTER_ENABLE_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()
