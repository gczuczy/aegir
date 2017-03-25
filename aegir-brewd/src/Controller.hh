/*
 * This object is the main controller, the decision making logic
 */

#ifndef AEGIR_CONTROLLER_H
#define AEGIR_CONTROLLER_H

#include "ThreadManager.hh"
#include "ZMQ.hh"

namespace aegir {

  class Controller: public ThreadBase {
  private:
    Controller();
    Controller(Controller &&) = delete;
    Controller(const Controller &) = delete;
    Controller &operator=(Controller &&) = delete;
    Controller &operator=(const Controller &) = delete;

    struct pinstate {
      std::string name;
      bool state;
    };

  public:
    virtual ~Controller();
    static Controller *getInstance();
    virtual void run();

  private:
    static Controller *c_instance;
    ZMQ::Socket c_mq_io, c_mq_iocmd;
  };
}

#endif
