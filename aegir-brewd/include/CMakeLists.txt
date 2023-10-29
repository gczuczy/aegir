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
  cmakeconfig.hh
  TSDB.hh
  types.hh
)

# disabled due to
# https://reviews.llvm.org/D18557
#target_precompile_headers(brewd
#  PRIVATE ${brewd_HEADERS}
#)

target_sources(brewd
  PRIVATE
  ${brewd_HEADERS}
)

target_sources(tests
  PRIVATE
  ${brewd_HEADERS}
)
