#include "Controller.hh"

#include <time.h>

#include <cmath>

#include <map>
#include <set>
#include <string>

#include "Exception.hh"

namespace aegir {
  /*
   * Controller
   */

  Controller *Controller::c_instance(0);

  Controller::Controller(): PINTracker(), c_mq_io(ZMQ::SocketType::SUB), c_ps(ProcessState::getInstance()),
			    c_mq_iocmd(ZMQ::SocketType::PUB), c_stoprecirc(false) {
    // subscribe to our publisher for IO events
    try {
      c_mq_io.connect("inproc://iopub").subscribe("");
      c_mq_iocmd.connect("inproc://iocmd");
    }
    catch (std::exception &e) {
      printf("Sub failed: %s\n", e.what());
    }
    catch (...) {
      printf("Sub failed: unknown exception\n");
    }

    c_cfg = Config::getInstance();

    // reconfigure state variables
    reconfigure();

    // state change callback
    c_ps.registerStateChange(std::bind(&Controller::onStateChange, this, std::placeholders::_1, std::placeholders::_2));

    // the stage handlers
    c_stagehandlers[ProcessState::States::Empty] = std::bind(&Controller::stageEmpty,
							     std::placeholders::_1, std::placeholders::_2);
    c_stagehandlers[ProcessState::States::Loaded] = std::bind(&Controller::stageLoaded
							      , std::placeholders::_1, std::placeholders::_2);
    c_stagehandlers[ProcessState::States::PreWait] = std::bind(&Controller::stagePreWait,
							       std::placeholders::_1, std::placeholders::_2);
    c_stagehandlers[ProcessState::States::PreHeat] = std::bind(&Controller::stagePreHeat,
							       std::placeholders::_1, std::placeholders::_2);
    c_stagehandlers[ProcessState::States::NeedMalt] = std::bind(&Controller::stageNeedMalt,
								std::placeholders::_1, std::placeholders::_2);
    c_stagehandlers[ProcessState::States::Mashing] = std::bind(&Controller::stageMashing,
							       std::placeholders::_1, std::placeholders::_2);
    c_stagehandlers[ProcessState::States::Sparging] = std::bind(&Controller::stageSparging,
								std::placeholders::_1, std::placeholders::_2);
    c_stagehandlers[ProcessState::States::PreBoil] = std::bind(&Controller::stagePreBoil,
							       std::placeholders::_1, std::placeholders::_2);
    c_stagehandlers[ProcessState::States::Hopping] = std::bind(&Controller::stageHopping,
							       std::placeholders::_1, std::placeholders::_2);
    c_stagehandlers[ProcessState::States::Cooling] = std::bind(&Controller::stageCooling,
							       std::placeholders::_1, std::placeholders::_2);
    c_stagehandlers[ProcessState::States::Finished] = std::bind(&Controller::stageFinished, std::placeholders::_1,
								std::placeholders::_2);

      // finally register to the threadmanager
    auto thrmgr = ThreadManager::getInstance();
    thrmgr->addThread("Controller", *this);
  }

  Controller::~Controller() {
  }

  Controller *Controller::getInstance() {
    if ( !c_instance) c_instance = new Controller();
    return c_instance;
  }

  void Controller::run() {
    printf("Controller started\n");

    c_mythread = std::this_thread::get_id();

    // The main event loop
    std::shared_ptr<Message> msg;
    std::chrono::microseconds ival(20000);
    while ( c_run ) {

      // PINTracker's cycle
      startCycle();
      // read the GPIO pin channel first
      try {
	while ( (msg = c_mq_io.recv()) != nullptr ) {
	  if ( msg->type() == MessageType::PINSTATE ) {
	    auto psmsg = std::static_pointer_cast<PinStateMessage>(msg);
#ifdef AEGIR_DEBUG
	    printf("Controller: received %s:%hhu\n", psmsg->getName().c_str(), psmsg->getState());
#endif
	    try {
	      setPIN(psmsg->getName(), psmsg->getState());
	    }
	    catch (Exception &e) {
	      printf("No such pin: '%s' %lu\n", psmsg->getName().c_str(), psmsg->getName().length());
	      continue;
	    }
	  } else if ( msg->type() == MessageType::THERMOREADING ) {
	    auto trmsg = std::static_pointer_cast<ThermoReadingMessage>(msg);
#ifdef AEGIR_DEBUG
	    printf("Controller: got temp reading: %s/%f/%u\n",
		   trmsg->getName().c_str(),
		   trmsg->getTemp(),
		   trmsg->getTimestamp());
#endif
	    // add it to the process state
	    c_ps.addThermoReading(trmsg->getName(), trmsg->getTimestamp(), trmsg->getTemp());
	  } else {
	    printf("Got unhandled message type: %i\n", (int)msg->type());
	    continue;
	  }
	}
      } // End of pin and sensor readings
      catch (Exception &e) {
	printf("Exception: %s\n", e.what());
      }

      // handle the state changes
      {
	std::lock_guard<std::mutex> g(c_mtx_stchqueue);
	if ( c_stchqueue.size() ) {
	  for ( auto &it: c_stchqueue ) {
	    onStateChange(it.first, it.second);
	  }
	  c_stchqueue.clear();
	}
      }

      // handle the changes
      controlProcess(*this);

      // Send back the commands to IOHandler
      // PINTracker::endCycle() does this
      endCycle();

      // sleep a bit
      std::this_thread::sleep_for(ival);
    }

    printf("Controller stopped\n");
  }

  void Controller::reconfigure() {
    PINTracker::reconfigure();
    c_hecycletime = c_cfg->getHECycleTime();
  }

  void Controller::controlProcess(PINTracker &_pt) {
    ProcessState::Guard guard_ps(c_ps);
    ProcessState::States state = c_ps.getState();

    // The pump&heat control button
    if ( _pt.hasChanges() ) {
      std::shared_ptr<PINTracker::PIN> swon(_pt.getPIN("swon"));
      std::shared_ptr<PINTracker::PIN> swoff(_pt.getPIN("swoff"));
      // whether we have at least one of the controls
      if ( swon->isChanged() || swoff->isChanged() ) {
	if ( swon->getNewValue() != PINState::Off
	     && swoff->getNewValue() == PINState::Off ) {
	  setPIN("swled", PINState::On);
	  c_stoprecirc = true;
	} else if ( swon->getNewValue() == PINState::Off &&
		    swoff->getNewValue() != PINState::Off ) {
	  setPIN("swled", PINState::Off);
	  c_stoprecirc = false;
	}
      }
    } // pump&heat switch

    // state function
    auto it = c_stagehandlers.find(state);
    if ( it != c_stagehandlers.end() ) {
      it->second(this, _pt);
    } else {
      printf("No function found for state %hhu\n", state);
    }

    if ( c_stoprecirc ) {
      setPIN("rimspump", PINState::Off);
      setPIN("rimsheat", PINState::Off);
    } // stop recirc

  } // controlProcess

  void Controller::onStateChange(ProcessState::States _old, ProcessState::States _new) {

    // if it's not in our thread, then queue the call and return
    if ( std::this_thread::get_id() != c_mythread ) {
      std::lock_guard<std::mutex> g(c_mtx_stchqueue);
      c_stchqueue.push_back(std::pair<ProcessState::States, ProcessState::States>(_old, _new));
      return;
    }

    c_lastcontrol = 0;

    if ( _new == ProcessState::States::NeedMalt ) {
	setPIN("buzzer", PINState::Pulsate, 2.1f, 0.4f);
    }

    if ( _old == ProcessState::States::NeedMalt ) {
      setPIN("buzzer", PINState::Off);
    }

    if ( _new == ProcessState::States::Mashing ) {
      setPIN("buzzer", PINState::Off);
      c_ps.setMashStep(-1);
      c_ps.setMashStepStart(0);
    }
}

  void Controller::stageEmpty(PINTracker &_pt) {
  }

  void Controller::stageLoaded(PINTracker &_pt) {
    c_prog = c_ps.getProgram();
    // Loaded, so we should verify the timestamps
    uint32_t startat = c_ps.getStartat();
    c_hecycletime = c_cfg->getHECycleTime();
    // if we start immediately then jump to PreHeat
    if ( startat == 0 ) {
      c_ps.setState(ProcessState::States::PreHeat);
    } else {
      c_ps.setState(ProcessState::States::PreWait);
    }
    return;
  }

  void Controller::stagePreWait(PINTracker &_pt) {
    float mttemp = c_ps.getSensorTemp("MashTun");
    if ( mttemp == 0 ) return;
    // let's see how much time do we have till we have to start pre-heating
    uint32_t now = time(0);
    // calculate how much time
    float tempdiff = c_prog->getStartTemp() - mttemp;
    // Pre-Heat time
    // the time we have till pre-heating has to actually start
    uint32_t phtime = 0;

    if ( tempdiff > 0 )
      phtime = calcHeatTime(c_ps.getVolume(), tempdiff, 0.001*c_cfg->getHEPower());

    printf("Controller::controllProcess() td:%.2f PreHeatTime:%u\n", tempdiff, phtime);
    uint32_t startat = c_ps.getStartat();
    if ( (now + phtime*1.15) > startat ) {
      c_ps.setState(ProcessState::States::PreHeat);
    }
  }

  void Controller::stagePreHeat(PINTracker &_pt) {
    float mttemp = c_ps.getSensorTemp("MashTun");
    float rimstemp = c_ps.getSensorTemp("RIMS");
    float targettemp = c_prog->getStartTemp();
    //    float tempdiff = targettemp - mttemp;

    tempControl(targettemp, 6);
    if ( mttemp >= targettemp ) {
	c_ps.setState(ProcessState::States::NeedMalt);
    }
  }

  void Controller::stageNeedMalt(PINTracker &_pt) {
    // When the malts are added, temperature decreases
    // We have to heat it back up to the designated temperature
    float targettemp = c_prog->getStartTemp();
    tempControl(targettemp, c_cfg->getHeatOverhead());
  }

  void Controller::stageMashing(PINTracker &_pt) {
    const Program::MashSteps &steps = c_prog->getMashSteps();

    int8_t msno = c_ps.getMashStep();
    float mttemp = c_ps.getSensorTemp("MashTun");
    int nsteps = steps.size();
    if ( msno >= nsteps ) {
      // we'll go to sparging here, but first we go up to endtemp
      float endtemp = c_prog->getEndTemp();

      tempControl(endtemp, c_cfg->getHeatOverhead());
      if ( mttemp >= endtemp ) {
	c_ps.setState(ProcessState::States::Sparging);
      }
      return;
    }

    // here we are doing the steps
    Program::MashStep ms(steps[msno<0?0:msno]);
    time_t now = time(0);
    // if it's <0, then we still have to get to
    // the first step's temperature
    if ( msno < 0 ) {
      tempControl(ms.temp, c_cfg->getHeatOverhead());

      if ( mttemp >= ms.temp - c_cfg->getTempAccuracy() ) {
	c_ps.setMashStep(0);
	c_ps.setMashStepStart(now);
	printf("Controller::stageMashing(): starting step 0 at %.2f/%.2f\n", mttemp, ms.temp);
	return;
      }
    } else {
      // here we are doing the regular steps
      // heating up to the next step, is the responsibility of
      // the previous one, hence the -1 previously, heating up to the 0th step
      time_t start = c_ps.getMashStepStart();
      float target = ms.temp;
      time_t held = now - start;

      // we held it for long enough, now we're aiming for the next step
      // and if we've reached it, we bump the step
      if ( held > ms.holdtime*60 ) {
	target = ((msno+1)<nsteps) ? steps[msno+1].temp : c_prog->getStartTemp();
	if ( mttemp + c_cfg->getTempAccuracy() > target) {
	  c_ps.setMashStep(msno+1);
	  c_ps.setMashStepStart(now);
	  printf("Controller::stageMashing(): step %i->%i\n", msno, msno+1);
	}
      }

      static time_t lastreport;
      if ( lastreport != now && now % 5 == 0 ) {
	printf("Controller::stageMashing(): S:%hhi HT:%li/%i MT:%.2f T:%.2f\n",
	       msno, held, ms.holdtime, mttemp, target);
	lastreport = now;
      }
      tempControl(target, c_cfg->getHeatOverhead());
    }
  }

  void Controller::stageSparging(PINTracker &_pt) {
    printf("%s:%i:%s\n", __FILE__, __LINE__, __FUNCTION__);
    float endtemp = c_prog->getEndTemp();
    tempControl(endtemp, c_cfg->getHeatOverhead());
  }

  void Controller::stagePreBoil(PINTracker &_pt) {
    printf("%s:%i:%s\n", __FILE__, __LINE__, __FUNCTION__);
  }

  void Controller::stageHopping(PINTracker &_pt) {
    printf("%s:%i:%s\n", __FILE__, __LINE__, __FUNCTION__);
  }

  void Controller::stageCooling(PINTracker &_pt) {
    printf("%s:%i:%s\n", __FILE__, __LINE__, __FUNCTION__);
  }

  void Controller::stageFinished(PINTracker &_pt) {
    printf("%s:%i:%s\n", __FILE__, __LINE__, __FUNCTION__);
  }


  void Controller::handleOutPIN(PINTracker::PIN &_pin) {
    c_mq_iocmd.send(PinStateMessage(_pin.getName(),
				    _pin.getNewValue(),
				    _pin.getNewCycletime(),
				    _pin.getNewOnratio()));
  }

  void Controller::tempControl(float _target, float _maxoverheat) {
    float mttemp = c_ps.getSensorTemp("MashTun");
    float rimstemp = c_ps.getSensorTemp("RIMS");

    c_ps.setTargetTemp(_target);

    if ( mttemp == 0.0f || rimstemp == 0.0f ) return;

    float tgtdiff = _target - mttemp;
    float rimsdiff = rimstemp - _target;

    time_t now = time(0);

    if ( getPIN("rimspump")->getOldValue() != PINState::On )
      setPIN("rimspump", PINState::On);

    // only control every 3 seconds
    if ( now - c_lastcontrol < std::ceil(c_hecycletime) ) return;

    if ( mttemp > _target ) {
      setPIN("rimsheat", PINState::Off);
      return;
    }
    c_lastcontrol = now;

    // coefficient based on the target temperature different
    float coeff_tgt=0;
    // coefficient based on the rims-mashtun tempdiff
    float coeff_rt=1;

    float halfpi = std::atan(INFINITY);
    if ( tgtdiff  > 0 ) {
      coeff_tgt = std::atan(tgtdiff)/halfpi;
      coeff_tgt = std::pow(coeff_tgt, 0.91);
    }
    if ( rimsdiff > 0 ) {
      coeff_rt = std::atan(_maxoverheat-rimsdiff)/halfpi;
      if ( coeff_rt < 0 ) coeff_rt = 0;
      else coeff_rt = std::pow(coeff_rt, 1.0/4);
    }

    float heratio = std::pow(coeff_tgt * coeff_rt, 0.71);
#define AEGIR_DEBUG_TEMPCTRL
#ifdef AEGIR_DEBUG_TEMPCTRL
    printf("Controller::tempControl(): MT:%.2f RIMS:%.2f TGT:%.2f d_tgt:%.2f d_rims:%.2f c_tgt:%.2f c_rt:%.2f HEr:%.2f\n",
	   mttemp, rimstemp, _target, tgtdiff, rimsdiff, coeff_tgt, coeff_rt, heratio);
#endif

    auto pin_he = getPIN("rimsheat");
    float absherdiff = std::abs(pin_he->getOldOnratio()-heratio);
#ifdef AEGIR_DEBUG_TEMPCTRL
    printf("Controller::tempControl(): rimsheat: ST:%hhu CT:%.2f OR:%.2f AHD:%.2f\n",
	   pin_he->getValue(), pin_he->getOldCycletime(), pin_he->getOldOnratio(), absherdiff);
#endif

    if (  absherdiff > 0.03 ) {
      setPIN("rimsheat", PINState::Pulsate, c_hecycletime, heratio);
    }
  }

  uint32_t Controller::calcHeatTime(uint32_t _vol, uint32_t _tempdiff, float _pkw) const {
    return (4.2 * _vol * _tempdiff)/_pkw;
  }
}
