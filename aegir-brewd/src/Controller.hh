/*
 * This object is the main controller, the decision making logic
 */

#ifndef AEGIR_CONTROLLER_H
#define AEGIR_CONTROLLER_H

#include <time.h>

#include <map>
#include <functional>
#include <mutex>
#include <thread>
#include <list>
#include <utility>

#include "ThreadManager.hh"
#include "PINTracker.hh"
#include "ZMQ.hh"
#include "ProcessState.hh"
#include "Config.hh"

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
    uint32_t calcHeatTime(uint32_t _vol, uint32_t _tempdiff, float _pkw) const;

    // control stages
    typedef std::function<void(Controller*, PINTracker&)> stagefunc_t;
    std::map<ProcessState::States, stagefunc_t> c_stagehandlers;
    void onStateChange(ProcessState::States _old, ProcessState::States _new);
    void stageEmpty(PINTracker &_pt);
    void stageLoaded(PINTracker &_pt);
    void stagePreWait(PINTracker &_pt);
    void stagePreHeat(PINTracker &_pt);
    void stageNeedMalt(PINTracker &_pt);
    void stageMashing(PINTracker &_pt);
    void stageSparging(PINTracker &_pt);
    void stagePreBoil(PINTracker &_pt);
    void stageHopping(PINTracker &_pt);
    void stageCooling(PINTracker &_pt);
    void stageFinished(PINTracker &_pt);
    //    void stage();

    // tempareture control
    void setTempTarget(float _target, float _maxoverheat);
    int tempControl();
    bool getTemps(const ProcessState::ThermoDataPoints &_tdp, uint32_t _dt, float &_last, float &_curr, float &_dT);

  private:
    static Controller *c_instance;
    ZMQ::Socket c_mq_io, c_mq_iocmd;
    std::thread::id c_mythread;
    std::mutex c_mtx_stchqueue;
    std::list<std::pair<ProcessState::States, ProcessState::States> > c_stchqueue;
    bool c_stoprecirc;
    time_t c_lastcontrol;
    float c_last_flow_volume;
    ProcessState &c_ps;
    std::shared_ptr<Program> c_prog;
    Config *c_cfg;
    float c_hecycletime;
    bool c_needcontrol; // whether to do tempcontrols
    float c_temptarget;
    bool c_newtemptarget;
    float c_tempoverheat;
  };
}

#endif
