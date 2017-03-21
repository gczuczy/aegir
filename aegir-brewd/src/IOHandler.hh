/*
  This class implements handling operating the GPIO devices
 */

#ifndef AEGIR_IOHANDLER_H
#define AEGIR_IOHANDLER_H

#include "ThreadManager.hh"

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

  public:
    static IOHandler *getInstance();
    virtual void run();
  };
}


#endif
