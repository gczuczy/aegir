list(APPEND fermd_HEADERS
  FermdConfig.hh
  MainLoop.hh
  Bluetooth.hh
  ZMQConfig.hh
  Message.hh
  SensorProxy.hh
  DB.hh
)

add_subdirectory(tests)

#list(APPEND fermd_HEADERS
#  cmakeconfig.hh
#)

add_custom_command(
  OUTPUT sql_v1_gen.cc
  COMMAND xxd -i sql/v1.sql > sql_v1_gen.cc
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/sql/v1.sql
  VERBATIM
)
add_custom_target(embed_generators
  DEPENDS sql_v1_gen.cc
)
add_dependencies(fermd embed_generators)
add_dependencies(tests_fermd embed_generators)

target_sources(fermd
  PRIVATE
  main.cc
  FermdConfig.cc
  MainLoop.cc
  Bluetooth.cc
  ZMQConfig.cc
  Message.cc
  SensorProxy.cc
  DB.cc
  sql_v1_gen.cc
  ${fermd_HADERS}
)

target_sources(tests_fermd
  PRIVATE
  FermdConfig.cc
  MainLoop.cc
  Bluetooth.cc
  ZMQConfig.cc
  Message.cc
  SensorProxy.cc
  DB.cc
  sql_v1_gen.cc
  ${brewd_HEADERS}
)
