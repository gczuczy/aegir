/*
 * PR thread, AKA Public Relations
 * This thread is responsible for communication with the REST API
 */

#ifndef AEGIR_PRWORKERTHREAD_H
#define AEGIR_PRWORKERTHREAD_H

#include <jsoncpp/json/json.h>
#include <memory>
#include <functional>
#include <map>
#include <string>

#include "ThreadManager.hh"
#include "ZMQ.hh"

namespace aegir {

  class PRWorkerThread: public ThreadBase {
    PRWorkerThread() = delete;
    //    PRWorkerThread(PRWorkerThread &&) = delete;
    PRWorkerThread(const PRWorkerThread &) = delete;
    PRWorkerThread &operator=(PRWorkerThread &&) = delete;
    PRWorkerThread &operator=(const PRWorkerThread &) = delete;
  public:
    PRWorkerThread(std::string _name);
    virtual ~PRWorkerThread();

  public:
    virtual void run();

  private:
    std::string c_name;
    ZMQ::Socket c_mq_prw, c_mq_iocmd;
    std::map<std::string, std::function<std::shared_ptr<Json::Value> (const Json::Value&) > > c_handlers;

  private:
    std::shared_ptr<Json::Value> handleJSONMessage(const Json::Value &_msg);
    std::shared_ptr<Json::Value> handleLoadProgram(const Json::Value &_data);
    std::shared_ptr<Json::Value> handleGetLoadedProgram(const Json::Value &_data);
    std::shared_ptr<Json::Value> handleGetState(const Json::Value &_data);
    std::shared_ptr<Json::Value> handleBuzzer(const Json::Value &_data);
    std::shared_ptr<Json::Value> handleHasMalt(const Json::Value &_data);
    std::shared_ptr<Json::Value> handleSpargeDone(const Json::Value &_data);
    std::shared_ptr<Json::Value> handleStartHopping(const Json::Value &_data);
    std::shared_ptr<Json::Value> handleResetProcess(const Json::Value &_data);
    std::shared_ptr<Json::Value> handleGetVolume(const Json::Value &_data);
    std::shared_ptr<Json::Value> handleSetVolume(const Json::Value &_data);
    std::shared_ptr<Json::Value> handleGetTempHistory(const Json::Value &_data);
  };
}

#endif
