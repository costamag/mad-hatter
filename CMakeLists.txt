cmake_minimum_required(VERSION 3.16)
project(mad_hatter)

set(CMAKE_CXX_STANDARD 17)

# Add dependencies
add_subdirectory(lib/mockturtle)
add_subdirectory(lib/replxx)

# CLI executable
add_executable(hatter src/main.cpp)
target_link_libraries(hatter PRIVATE mockturtle replxx)

