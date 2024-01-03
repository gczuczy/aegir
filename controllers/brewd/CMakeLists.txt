list(APPEND brewd_HEADERS
  Config.hh
  Controller.hh
  DirectSelect.hh
  ElapsedTime.hh
  Exception.hh
  GPIO.hh
  IOHandler.hh
  JSONMessage.hh
  MAX31856.hh
  Message.hh
  PINTracker.hh
  PRThread.hh
  PRWorkerThread.hh
  ProcessState.hh
  Program.hh
  SPI.hh
  ThreadManager.hh
  ZMQ.hh
  TSDB.hh
  types.hh
  Environment.hh
  logging.hh
  LogChannel.hh
)

# disabled due to
# https://reviews.llvm.org/D18557
#target_precompile_headers(brewd
#  PRIVATE ${brewd_HEADERS}
#)

target_sources(brewd
  PRIVATE
  LogChannel.cc
  Config.cc
  Controller.cc
  DirectSelect.cc
  ElapsedTime.cc
  Exception.cc
  GPIO.cc
  IOHandler.cc
  JSONMessage.cc
  MAX31856.cc
  Message.cc
  PINTracker.cc
  PRThread.cc
  PRWorkerThread.cc
  ProcessState.cc
  Program.cc
  SPI.cc
  ThreadManager.cc
  ZMQ.cc
  main.cc
  TSDB.cc
  types.cc
  Environment.cc
  logging.cc
  ${brewd_HEADERS}
)

target_sources(tests
  PRIVATE
  TSDB.cc
  Exception.cc
  types.cc
  Config.cc
  Message.cc
  logging.cc
  ${brewd_HEADERS}
)
