include(FetchContent)

# Fetch Catch2 v3.x
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.0
)
FetchContent_MakeAvailable(Catch2)

# Collect test sources
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS *.cpp)

add_executable(run_tests ${TEST_SOURCES})
target_include_directories(run_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_link_libraries(run_tests PRIVATE mockturtle Catch2::Catch2WithMain)

include(Catch)
if (NOT MSVC)
  include(CTest)
  include(Catch)
  catch_discover_tests(run_tests)
endif()
