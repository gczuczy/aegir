/*
 * PR thread, AKA Public Relations
 * This thread is responsible for communication with the REST API
 */

#ifndef AEGIR_PRTHREAD_H
#define AEGIR_PRTHREAD_H

#include "ThreadManager.hh"
#include "ZMQ.hh"

namespace aegir {

  class PRThread: public ThreadBase {
    //    PRThread(PRThread &&) = delete;
    PRThread(const PRThread &) = delete;
    PRThread &operator=(PRThread &&) = delete;
    PRThread &operator=(const PRThread &) = delete;
  public:
    PRThread();
    virtual ~PRThread();

  public:
    virtual void run();

  private:
    ZMQ::Socket c_mq_pr;
  };
}

#endif
