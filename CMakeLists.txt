cmake_minimum_required(VERSION 3.16)
project(mad_hatter)

# Options
option(HATTER_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(HATTER_ENABLE_TESTS "Build unit tests" ON)

# Sanitizer
if (HATTER_ENABLE_ASAN)
  set(SANITIZE_FLAGS "-fsanitize=address -fno-omit-frame-pointer")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZE_FLAGS}")
  set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} ${SANITIZE_FLAGS}")
endif()

set(CMAKE_CXX_STANDARD 17)

# SIMD checks
if(UNIX)
  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag("-mavx2" HAS_MAVX2)
  check_cxx_compiler_flag("-mfma" HAS_MFMA)
  if(HAS_MAVX2 AND HAS_MFMA)
    message(STATUS "[mad_hatter] Enabling AVX2 for SIMD operations")
    add_compile_options(-mavx2 -mfma)
  endif()
endif()

if(MSVC)
  add_compile_options(/arch:AVX2)
endif()

# Dependencies
add_subdirectory(lib/mockturtle)
add_subdirectory(lib/replxx)
add_subdirectory(lib/cli11)

# Shared CLI logic as a library (exclude main.cpp)
file(GLOB_RECURSE MAD_CLI_SOURCES CONFIGURE_DEPENDS
    src/cli/*.cpp
)
list(FILTER MAD_CLI_SOURCES EXCLUDE REGEX "src/cli/main.cpp$")

add_library(mad_cli ${MAD_CLI_SOURCES})

# Link CLI logic against mockturtle to inherit fmt and other includes
target_link_libraries(mad_cli
    PUBLIC
        mockturtle
        replxx
        CLI11::CLI11
)

# Add include paths for CLI headers
target_include_directories(mad_cli
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src   # so "cli/context.hpp" works
)

# CLI executable
add_executable(hatter src/cli/main.cpp)
target_link_libraries(hatter PRIVATE mad_cli)

# Public header-only interface library
add_library(mad_hatter INTERFACE)
target_include_directories(mad_hatter INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Tests
if (HATTER_ENABLE_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()
