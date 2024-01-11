list(APPEND fermd_HEADERS
  FermdConfig.hh
  TiltDB.hh
)

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

target_sources(tests
  PRIVATE
  FermdConfig.cc
  TiltDB.cc
  ${brewd_HEADERS}
)
