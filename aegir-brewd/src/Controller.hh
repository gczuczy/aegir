/*
 * This object is the main controller, the decision making logic
 */

#ifndef AEGIR_CONTROLLER_H
#define AEGIR_CONTROLLER_H

#include <map>
#include <functional>

#include "ThreadManager.hh"
#include "PINTracker.hh"
#include "ZMQ.hh"
#include "ProcessState.hh"

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

  private:
    static Controller *c_instance;
    ZMQ::Socket c_mq_io, c_mq_iocmd;
    bool c_stoprecirc;
    int c_preheat_phase;
    ProcessState &c_ps;
    std::shared_ptr<Program> c_prog;
  };
}

#endif
