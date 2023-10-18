/*
 * PR thread, AKA Public Relations
 * This thread is responsible for communication with the REST API
 */

#ifndef AEGIR_PRTHREAD_H
#define AEGIR_PRTHREAD_H

#include <set>
#include <string>

#include "ThreadManager.hh"
#include "ZMQ.hh"

namespace aegir {

  class PRWorkerThread;

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
    virtual void stop() noexcept;

  private:
    ZMQ::Socket c_mq_pr, c_mq_prw, c_mq_prctrl, c_mq_prdbg;
    std::set<PRWorkerThread*> c_workers;
  };
}

#endif
