list(APPEND fermd_HEADERS
  FermdConfig.hh
  TiltDB.hh
)

add_subdirectory(tests)

#list(APPEND fermd_HEADERS
#  cmakeconfig.hh
#)

target_sources(fermd
  PRIVATE
  main.cc
  FermdConfig.cc
  TiltDB.cc
  ${fermd_HADERS}
)

target_sources(tests_fermd
  PRIVATE
  FermdConfig.cc
  TiltDB.cc
  ${brewd_HEADERS}
)
