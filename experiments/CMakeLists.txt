# experiments/CMakeLists.txt

# Recursively find all experiment .cpp files
file(GLOB_RECURSE EXPERIMENT_CPP CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

# Shared sources (e.g., common utilities)
file(GLOB_RECURSE EXP_COMMON_CPP CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/common/*.cpp
)

foreach(SRC ${EXPERIMENT_CPP})
  # Get relative path like "rw/exp_cut.cpp"
  file(RELATIVE_PATH REL ${CMAKE_CURRENT_SOURCE_DIR} ${SRC})

  # Skip anything under "common/"
  string(FIND "${REL}" "common/" COMMON_POS)
  if (NOT COMMON_POS EQUAL -1)
    continue()
  endif()

  # Generate target name like "exp_rw_exp_cut"
  get_filename_component(NAME_WE ${REL} NAME_WE)
  get_filename_component(DIR ${REL} DIRECTORY)

  if (DIR)
    string(REPLACE "/" "_" DIR_UNDERSCORE ${DIR})
    set(TGT "exp_${DIR_UNDERSCORE}_${NAME_WE}")
  else()
    set(TGT "exp_${NAME_WE}")
  endif()

  add_executable(${TGT} ${SRC} ${EXP_COMMON_CPP})
  set_target_properties(${TGT} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/experiments
  )

  target_include_directories(${TGT} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_SOURCE_DIR}/src
  )

  target_link_libraries(${TGT} PRIVATE rho_cli rinox_build_options)
endforeach()
