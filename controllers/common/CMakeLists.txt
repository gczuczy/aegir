
add_subdirectory(tests)

list(APPEND brewd_HEADERS
  cmakeconfig.hh
  Exception.hh
  logging.hh
  LogChannel.hh
  misc.hh
)

list(APPEND fermd_FILES
  cmakeconfig.hh
  ServiceManager.hh
  ServiceManager.cc
  ConfigBase.hh
  ConfigBase.cc
  Exception.hh
  Exception.cc
  logging.hh
  logging.cc
  LogChannel.hh
  LogChannel.cc
  misc.hh
  misc.cc
  ryml.hh
  ThreadManager.hh
  ThreadManager.cc
  ZMQ.hh
  ZMQ.cc
  Message.hh
  Message.cc
  ryml.cc
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
  ${fermd_FILES}
)

target_sources(tests_fermd
  PRIVATE
  ${fermd_FILES}
)
