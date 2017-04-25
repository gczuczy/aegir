/*
 * This object is the main controller, the decision making logic
 */

#ifndef AEGIR_CONTROLLER_H
#define AEGIR_CONTROLLER_H

#include "ThreadManager.hh"
#include "PINTracker.hh"
#include "ZMQ.hh"

namespace aegir {

  class Controller: public ThreadBase, public PINTracker {
  private:
    Controller();
    Controller(Controller &&) = delete;
    Controller(const Controller &) = delete;
    Controller &operator=(Controller &&) = delete;
    Controller &operator=(const Controller &) = delete;

  public:
    virtual ~Controller();
    static Controller *getInstance();
    virtual void run() override;

  private:
    void reconfigure();
    void controlProcess(PINTracker &_pt);
    virtual void handleOutPIN(PINTracker::PIN &_pin) override;

  private:
    static Controller *c_instance;
    ZMQ::Socket c_mq_io, c_mq_iocmd;
  };
}

#endif
