list(APPEND brewd_HEADERS
  cmakeconfig.hh
  Exception.hh
  logging.hh
  LogChannel.hh
  misc.hh
)

list(APPEND fermd_HEADERS
  cmakeconfig.hh
  Exception.hh
  logging.hh
  LogChannel.hh
  misc.hh
  ryml.hh
)

target_sources(brewd
  PRIVATE
  Exception.cc
  logging.cc
  LogChannel.cc
  ${brewd_HEADERS}
)

target_sources(fermd
  PRIVATE
  Exception.cc
  logging.cc
  LogChannel.cc
  ConfigBase.cc
  misc.cc
  ${fermd_HEADERS}
)

target_sources(tests_fermd
  PRIVATE
  Exception.hh
  Exception.cc
  logging.hh
  logging.cc
  cmakeconfig.hh
  ConfigBase.cc
  ConfigBase.hh
  misc.cc
  misc.hh
  ryml.hh
)
