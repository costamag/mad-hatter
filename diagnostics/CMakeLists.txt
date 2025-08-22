# diagnostics/CMakeLists.txt

file(GLOB_RECURSE RINOX_DIAG_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
)

# Public headers live directly under diagnostics/rinox/...
set(RINOX_DIAG_PUBLIC_DIR "${CMAKE_CURRENT_SOURCE_DIR}")  # contains the 'rinox/' dir

if (RINOX_DIAG_SOURCES STREQUAL "")
  add_library(rinox_diagnostics INTERFACE)
  target_include_directories(rinox_diagnostics
    INTERFACE
      $<BUILD_INTERFACE:${RINOX_DIAG_PUBLIC_DIR}>
      $<INSTALL_INTERFACE:include>           # install to <prefix>/include
  )
  target_compile_features(rinox_diagnostics INTERFACE cxx_std_17)

  # deps go through INTERFACE when header-only
  if (TARGET mockturtle::mockturtle)
    target_link_libraries(rinox_diagnostics INTERFACE mockturtle::mockturtle)
  elseif (TARGET mockturtle)
    target_link_libraries(rinox_diagnostics INTERFACE mockturtle)
  endif()
  if (TARGET lorina::lorina)
    target_link_libraries(rinox_diagnostics INTERFACE lorina::lorina)
  endif()
  if (TARGET fmt::fmt)
    target_link_libraries(rinox_diagnostics INTERFACE fmt::fmt)
  endif()

else()
  add_library(rinox_diagnostics STATIC ${RINOX_DIAG_SOURCES})
  target_include_directories(rinox_diagnostics
    PUBLIC
      $<BUILD_INTERFACE:${RINOX_DIAG_PUBLIC_DIR}>
      $<INSTALL_INTERFACE:include>
  )
  target_compile_features(rinox_diagnostics PUBLIC cxx_std_17)

  if (TARGET mockturtle::mockturtle)
    target_link_libraries(rinox_diagnostics PUBLIC mockturtle::mockturtle)
  elseif (TARGET mockturtle)
    target_link_libraries(rinox_diagnostics PUBLIC mockturtle)
  endif()
  if (TARGET lorina::lorina)
    target_link_libraries(rinox_diagnostics PUBLIC lorina::lorina)
  endif()
  if (TARGET fmt::fmt)
    target_link_libraries(rinox_diagnostics PUBLIC fmt::fmt)
  endif()
endif()

# Optional install rules (so #include <rinox/diagnostics.hpp> works after install)
# install(TARGETS rinox_diagnostics EXPORT rinoxTargets
#   ARCHIVE DESTINATION lib
#   LIBRARY DESTINATION lib
#   RUNTIME DESTINATION bin
#   INCLUDES DESTINATION include
# )
# install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/rinox" DESTINATION include)

set_property(TARGET rinox_diagnostics PROPERTY FOLDER "diagnostics")
