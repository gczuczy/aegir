/*
  This class implements handling operating the GPIO devices
 */

#ifndef AEGIR_IOHANDLER_H
#define AEGIR_IOHANDLER_H

#include "ThreadManager.hh"
#include "ZMQ.hh"
#include "SPI.hh"
#include "MAX31856.hh"

#include <vector>
#include <memory>
#include <map>
#include <string>

namespace aegir {

  class IOHandler: public ThreadBase {
    IOHandler() = delete;
    IOHandler(IOHandler&&) = delete;
    IOHandler(const IOHandler &) = delete;
    IOHandler &operator=(IOHandler &&) = delete;
    IOHandler &operator=(const IOHandler &) = delete;
  public:
    IOHandler(GPIO &_gpio, SPI &_spi);
    virtual ~IOHandler();

  private:
    GPIO &c_gpio;
    SPI &c_spi;
    ZMQ::Socket c_mq_pub;
    ZMQ::Socket c_mq_iocmd;
    std::vector<std::unique_ptr<MAX31856>> c_tcs;
    std::map<std::string, int> c_tcmap;
    uint32_t c_thermoival;

  private:
    void readTCs();

  public:
    virtual void run();
  };
}


#endif
