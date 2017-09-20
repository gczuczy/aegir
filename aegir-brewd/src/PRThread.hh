/*
 * PR thread, AKA Public Relations
 * This thread is responsible for communication with the REST API
 */

#ifndef AEGIR_PRTHREAD_H
#define AEGIR_PRTHREAD_H

#include <jsoncpp/json/json.h>
#include <memory>
#include <functional>
#include <map>
#include <string>

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
    ZMQ::Socket c_mq_pr, c_mq_iocmd;
    std::map<std::string, std::function<std::shared_ptr<Json::Value> (const Json::Value&) > > c_handlers;

  private:
    std::shared_ptr<Json::Value> handleJSONMessage(const Json::Value &_msg);
    std::shared_ptr<Json::Value> handleLoadProgram(const Json::Value &_data);
    std::shared_ptr<Json::Value> handleGetLoadedProgram(const Json::Value &_data);
    std::shared_ptr<Json::Value> handleGetState(const Json::Value &_data);
    std::shared_ptr<Json::Value> handleBuzzer(const Json::Value &_data);
    std::shared_ptr<Json::Value> handleHasMalt(const Json::Value &_data);
    std::shared_ptr<Json::Value> handleResetProcess(const Json::Value &_data);
  };
}

#endif
