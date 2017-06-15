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
#include <set>
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
    uint32_t c_pinival;
    // PIN holding structures
    std::map<std::string, PINState> c_inpins;
    std::set<std::string> c_outpins;

  private:
    void readTCs();
    void handlePins();

  public:
    virtual void run();
  };
}


#endif
