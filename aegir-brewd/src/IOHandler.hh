/*
  This class implements handling operating the GPIO devices
 */

#ifndef AEGIR_IOHANDLER_H
#define AEGIR_IOHANDLER_H

#include "ThreadManager.hh"
#include "ZMQ.hh"

namespace aegir {

  class IOHandler: public ThreadBase {
    IOHandler(IOHandler&&) = delete;
    IOHandler(const IOHandler &) = delete;
    IOHandler &operator=(IOHandler &&) = delete;
    IOHandler &operator=(const IOHandler &) = delete;
    IOHandler();
  public:
    virtual ~IOHandler();

  private:
    static IOHandler *c_instance;
    ZMQ::Socket c_mq_pub;
    ZMQ::Socket c_mq_iocmd;

  public:
    static IOHandler *getInstance();
    virtual void run();
  };
}


#endif
