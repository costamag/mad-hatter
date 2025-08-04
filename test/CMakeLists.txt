include(FetchContent)

#Fetch Catch2 v3.x
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.0
)
FetchContent_MakeAvailable(Catch2)

include(CTest)
include(Catch)

#-- -- Unit tests -- --
file(GLOB_RECURSE UNIT_SOURCES CONFIGURE_DEPENDS unit/*.cpp)

add_executable(run_unit_tests ${UNIT_SOURCES})
set_target_properties(run_unit_tests PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test
)
target_include_directories(run_unit_tests PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
  ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(run_unit_tests PRIVATE mad_cli Catch2::Catch2WithMain)

catch_discover_tests(run_unit_tests)

# ---- CLI tests ----
file(GLOB_RECURSE CLI_SOURCES CONFIGURE_DEPENDS cli/*.cpp)

add_executable(run_cli_tests ${CLI_SOURCES})
set_target_properties(run_cli_tests PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test
)
target_include_directories(run_cli_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(run_cli_tests PRIVATE mad_cli Catch2::Catch2WithMain)

catch_discover_tests(run_cli_tests)
