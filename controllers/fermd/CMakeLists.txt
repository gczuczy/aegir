list(APPEND fermd_HEADERS
  FermdConfig.hh
  TiltDB.hh
  MainLoop.hh
  Bluetooth.hh
  ZMQConfig.hh
  Message.hh
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
  MainLoop.cc
  Bluetooth.cc
  ZMQConfig.cc
  Message.cc
  SensorProxy.hh
  SensorProxy.cc
  ${fermd_HADERS}
)

target_sources(tests_fermd
  PRIVATE
  FermdConfig.cc
  TiltDB.cc
  MainLoop.cc
  Bluetooth.cc
  ZMQConfig.cc
  Message.cc
  ${brewd_HEADERS}
)
