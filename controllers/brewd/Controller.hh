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
#include <memory>

#include "ThreadManager.hh"
#include "PINTracker.hh"
#include "ZMQ.hh"
#include "ProcessState.hh"
#include "Config.hh"
#include "LogChannel.hh"

namespace aegir {

  class Controller: public ThreadBase, public PINTracker {
  private:
    class HERatioDB {
    public:
      struct alignas(sizeof(long)) data {
	time_t time;
	float ratio;
      };
    public:
      HERatioDB();
      HERatioDB(HERatioDB&) = delete;
      HERatioDB(const HERatioDB&) = delete;
      HERatioDB(HERatioDB&&) = delete;
      ~HERatioDB();

      HERatioDB& clear();
      HERatioDB& insert(float _value);

      const data& operator[](const std::size_t) const;
      inline const uint32_t size() const {return c_size;};

    private:
      void grow();

    private:
      std::uint32_t c_alloc_size;
      std::uint32_t c_capacity;
      std::uint32_t c_size;
      data *c_data;
    };

  private:
    Controller();
    Controller(Controller &&) = delete;
    Controller(const Controller &) = delete;
    Controller &operator=(Controller &&) = delete;
    Controller &operator=(const Controller &) = delete;

  public:
    virtual ~Controller();
    static std::shared_ptr<Controller> getInstance();
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
    void maintenanceMode(PINTracker &_pt);
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
    void stageTransfer(PINTracker &_pt);
    void stageFinished(PINTracker &_pt);
    //    void stage();

    // tempareture control
    void setTempTarget(float _target, float _maxoverheat);
    int tempControl();
    bool getTemps(ThermoCouple _tc, uint32_t _dt, float &_last, float &_curr, float &_dT);
    void setHERatio(float _cycletime, float _ratio);
    float calcFlowRate();

    inline float calcPower(float _dT, float _dt=60) {
      return (4.2 * c_ps.getVolume() * _dT)/ _dt;
    }

  private:
    ZMQ::Socket c_mq_io, c_mq_iocmd;
    std::thread::id c_mythread;
    std::mutex c_mtx_stchqueue;
    std::list<std::pair<ProcessState::States, ProcessState::States> > c_stchqueue;
    bool c_levelerror;
    time_t c_lastcontrol;
    float c_last_flow_volume;
    ProcessState &c_ps;
    std::shared_ptr<Program> c_prog;
    std::shared_ptr<Config> c_cfg;
    float c_hecycletime;
    bool c_needcontrol; // whether to do tempcontrols
    float c_temptarget;
    bool c_newtemptarget;
    float c_tempoverheat;
    HERatioDB c_heratiohistory;
    int32_t c_hestartdelay;
    bool c_hepause;
    LogChannel c_log;
    float c_correctionfactor;
  };
}

#endif
