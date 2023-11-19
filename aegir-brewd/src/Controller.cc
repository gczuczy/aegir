#include "Controller.hh"

#include <time.h>
#include <sys/event.h>
#include <unistd.h>

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <algorithm>

#include <map>
#include <set>
#include <string>

#include "Exception.hh"
#include "Environment.hh"

namespace aegir {
  /*
   * Controller::HERratioDB
   */
  Controller::HERatioDB::HERatioDB(): c_data(0), c_alloc_size(4096),
				      c_capacity(0), c_size(0) {
    c_data = (data*)std::malloc(c_alloc_size);
    c_capacity = c_alloc_size = sizeof(data);
    clear();
  };

  Controller::HERatioDB::~HERatioDB() {
    if ( c_data ) std::free(c_data);
  };

  Controller::HERatioDB& Controller::HERatioDB::clear() {
    std::memset((void*)c_data, 0, c_alloc_size);
    c_size = 0;
    return *this;
  }

  Controller::HERatioDB& Controller::HERatioDB::insert(float _value) {
    uint32_t idx = c_size++;

    c_data[idx].time = time(0);
    c_data[idx].ratio = _value;
    return *this;
  }

  const Controller::HERatioDB::data& Controller::HERatioDB::operator[](const std::size_t _at) const {
    if ( _at >= c_size )
      throw Exception("HERatioDB out of range (size:%lu at:%lu", c_size, _at);

    return c_data[_at];
  }

  void Controller::HERatioDB::grow() {
    uint32_t prevsize = c_alloc_size;
    uint32_t increment = getpagesize();
    c_alloc_size += increment;
    c_data = (data*)std::realloc((void*)c_data, c_alloc_size);
    c_capacity = c_alloc_size / sizeof(data);

    // and finally zero the newly allocated region
    void *start = (void*)(((uint8_t*)c_data)+prevsize);
    std::memset(start, 0, increment);
  }
  /*
   * Controller
   */

  Controller::Controller(): PINTracker(),
			    c_mq_io(ZMQ::SocketType::SUB),
			    c_ps(ProcessState::getInstance()),
			    c_mq_iocmd(ZMQ::SocketType::PUB),
			    c_levelerror(false), c_needcontrol(false),
			    c_hestartdelay(-1), c_hepause(false),
			    c_log("Controller") {
    // subscribe to our publisher for IO events
    try {
      c_mq_io.connect("inproc://iopub").subscribe("");
      c_mq_iocmd.connect("inproc://iocmd");
    }
    catch (std::exception &e) {
      c_log.error("Sub failed: %s", e.what());
    }
    catch (...) {
      c_log.error("Sub failed: unknown exception");
    }

    c_cfg = Config::getInstance();

    // reconfigure state variables
    reconfigure();

    // state change callback
    c_ps.registerStateChange(std::bind(&Controller::onStateChange, this, std::placeholders::_1, std::placeholders::_2));

    // the stage handlers
    c_stagehandlers[ProcessState::States::Maintenance] = std::bind(&Controller::maintenanceMode,
							     std::placeholders::_1, std::placeholders::_2);
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
    c_stagehandlers[ProcessState::States::Transfer] = std::bind(&Controller::stageTransfer, std::placeholders::_1,
								std::placeholders::_2);
    c_stagehandlers[ProcessState::States::Finished] = std::bind(&Controller::stageFinished, std::placeholders::_1,
								std::placeholders::_2);

      // finally register to the threadmanager
    auto thrmgr = ThreadManager::getInstance();
    thrmgr->addThread("Controller", *this);
  }

  Controller::~Controller() {
  }

  std::shared_ptr<Controller> Controller::getInstance() {
    static std::shared_ptr<Controller> instance{new Controller()};
    return instance;
  }

  void Controller::run() {
    c_log.info("Controller started");

    c_mythread = std::this_thread::get_id();

    int kq = kqueue();
    int kq_id_control = 1;
    int kq_id_temp = 2;
    int kqerr;
    struct kevent kevents[4];
    struct kevent kevchanges[4];

    //EV_SET(kev, ident, filter, flags, fflags, data, udata);
    EV_SET(&kevchanges[0], kq_id_control, EVFILT_TIMER, EV_ADD, NOTE_SECONDS, 1, 0);

    // register the events
    kqerr = kevent(kq, kevchanges, 1, 0, 0, 0);

    // The main event loop
    std::shared_ptr<Message> msg;
    std::set<int> events;
    int nevents;
    int nexttempcontrol = 1;
    bool tc_installed = false;	// tempcontrol timer installed
    while ( c_run ) {

      // first, gather the events
      // timeout=NULL, kevent blocks here until there's an event
      nevents = kevent(kq, 0, 0, kevents, 2, 0);

      // in case of errors, check again
      if ( nevents <= 0 ) continue;
      // gather the triggered events into a set
      // for easier checking later
      events.clear();
      for ( int i=0; i<nevents; ++i )
	events.insert(kevents[i].ident);

      // PINTracker's cycle
      startCycle();

      // read the GPIO PINs and SPI bus first
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
	      c_log.warn("Error on PIN '%s'/%lu: %s", psmsg->getName().c_str(), psmsg->getName().length(), e.what());
	      continue;
	    }
	  } else if ( msg->type() == MessageType::THERMOREADING ) {
	    auto trmsg = std::static_pointer_cast<ThermoReadingMessage>(msg);
	    // add it to the process state
	    c_ps.addThermoReadings(trmsg->getTimestamp(), trmsg->getTemps());
	  } else {
	    c_log.warn("Got unhandled message type: %i", (int)msg->type());
	    continue;
	  }
	}
      } // End of pin and sensor readings
      catch (Exception &e) {
	c_log.error("Controller::run exception: %s", e.what());
      }

      // state changes and controlling goes hand-in-hand
      ProcessState::States state, oldstate;
      state = c_ps.getState();
      do {
	oldstate = state;
	// handle the state changes
	{
	  // trigger the state change queue handler
	  std::lock_guard<std::mutex> g(c_mtx_stchqueue);
	  if ( c_stchqueue.size() ) {
	    for ( auto &it: c_stchqueue ) {
	      onStateChange(it.first, it.second);
	    }
	    c_stchqueue.clear();
	  }
	}

	// the control event
	// also run it after the tempcontrol
	if ( events.find(kq_id_control) != events.end() ) {
	  // handle the current state
	  controlProcess(*this);
	}
	state = c_ps.getState();
      } while ( state != oldstate );

      // check the tube's temperature, watch for overheating
      try {
	float rimstemp = Environment::getInstance()->getTempRIMS();
	if ( c_needcontrol && !c_hepause &&
	     rimstemp >= (c_temptarget+c_cfg->getHeatOverhead()*1.15) ) {
	  c_log.info("Pausing heat: RIMS:%.2f Target:%.2f Overhead:%.2f (%.2f)",
		     rimstemp, c_temptarget, c_cfg->getHeatOverhead(),
		     c_temptarget+c_cfg->getHeatOverhead());
	  c_hepause = true;
	  setPIN("mtheat", PINState::Pulsate, 5.0f, 0.01f);
	} else if ( c_needcontrol && c_hepause &&
		    rimstemp < (c_temptarget+c_cfg->getHeatOverhead()*0.95) ) {
	  c_log.info("hepause:off newtemptarget:true");
	  c_hepause = false;
	  c_newtemptarget = true;
	}
      }
      catch (Exception &e) {
	c_log.error("RIMS safety check failed: %s", e.what());
      }

      // the temp control event
      if ( !c_hepause &&
	   (events.find(kq_id_temp) != events.end() ||
	    (c_needcontrol && c_newtemptarget)) ) {
	c_log.debug("Running tempcontrol...");
	nexttempcontrol = tempControl();
	c_log.trace("Tempcontrol said %i secs", nexttempcontrol);
	if ( nexttempcontrol < 3 ) nexttempcontrol = 3;
	else if ( nexttempcontrol > 30 ) nexttempcontrol = 30;
	tc_installed = false;	// oneshot fired, needs reinstall
      }

      // if we need tempcontrols, then keep on
      // installing the oneshot event
      if ( c_needcontrol && !tc_installed && !c_hepause) {
	//EV_SET(kev, ident, filter, flags, fflags, data, udata);
	EV_SET(&kevchanges[0], kq_id_temp, EVFILT_TIMER, EV_ADD|EV_ONESHOT, NOTE_SECONDS, nexttempcontrol, 0);

	// register the events
	kqerr = kevent(kq, kevchanges, 1, 0, 0, 0);
	c_log.debug("Installed tempcontrol for %i secs", nexttempcontrol);
	tc_installed = true;
      }

      // if we don't need tempcontrol and the event is installed, remove it
      if ( !c_needcontrol && tc_installed ||
	   c_needcontrol && c_hepause && tc_installed) {
	//EV_SET(kev, ident, filter, flags, fflags, data, udata);
	EV_SET(&kevchanges[0], kq_id_temp, EVFILT_TIMER, EV_DELETE, 0, 0, 0);

	// register the events
	kqerr = kevent(kq, kevchanges, 1, 0, 0, 0);
	c_log.debug("Removed tempcontrol");
	tc_installed = false;
      }

      // TODO: REVISE, seprate the level error case
      // if the recirc button is pushed, or we don't need tempcontrol anymore
      // stop the pump and the heating element
      if ( c_levelerror || !c_needcontrol ) {
	if ( c_ps.getState() != ProcessState::States::Maintenance  ) {
	  if ( c_ps.getForceMTPump() ) {
	    setPIN("mtpump", PINState::On);
	  } else {
	    setPIN("mtpump", PINState::Off);
	  }
	}
	//printf("Setting mtheat off %i\n", __LINE__);
	setPIN("mtheat", PINState::Off);
      } // stop recirculation if we don't need control anymore, OR there's level error
      if ( c_needcontrol && c_ps.getBlockHeat() ) {
	c_log.warn("Setting mtheat off");
	setPIN("mtheat", PINState::Off);
      }

      // check whether the mtheat had just been turned on
      auto mtheat = getPIN("mtheat");
      if ( mtheat->getOldValue() == PINState::Off &&
	   mtheat->getNewValue() != PINState::Off ) {
	//printf("Starting hedelay\n");
	setPIN("mtheat", PINState::Pulsate, 5.0f, 0.01f);
	c_hestartdelay = c_cfg->getHEDelay();
      } else if ( c_hestartdelay > 0 ) {
	--c_hestartdelay;
	//printf("Still in hedelay: %i\n", c_hestartdelay);
	setPIN("mtheat", PINState::Pulsate, 5.0f, 0.01f);
      }

      // end the GPIO change cycle
      endCycle();
    } // while ( c_run )

    close(kq);

    c_mq_io.close();
    c_mq_iocmd.close();
    c_log.info("Controller stopped");
  }

  void Controller::reconfigure() {
    PINTracker::reconfigure();
    c_hecycletime = c_cfg->getHECycleTime();
    c_needcontrol = false;
  }

  void Controller::controlProcess(PINTracker &_pt) {
    ProcessState::Guard guard_ps(c_ps);
    ProcessState::States state = c_ps.getState();

    // if the MT water level meter signals, we're stopping the circulation
    if ( _pt.hasChanges() ) {
      std::shared_ptr<PINTracker::PIN> mtlvl(_pt.getPIN("mtlevel"));
      // handle the water level sensor in the MT
      if ( mtlvl->isChanged() ) {
	c_levelerror = (mtlvl->getNewValue() == PINState::Off);
	//printf("MTlevel changed to %s\n", c_levelerror?"error":"ok");
	c_ps.setLevelError(c_levelerror);
      }
    } // pump&heat switch

    // calling the state function
    // if the state changes after, run the next state function as well
    auto it = c_stagehandlers.find(state);
    if ( it != c_stagehandlers.end() ) {
      it->second(this, _pt);
    } else {
      c_log.warn("No function found for state %hhu", state);
    }

  } // controlProcess

  void Controller::onStateChange(ProcessState::States _old, ProcessState::States _new) {
    // if it's not in our thread, then queue the call and return
    if ( std::this_thread::get_id() != c_mythread ) {
      std::lock_guard<std::mutex> g(c_mtx_stchqueue);
      c_stchqueue.push_back(std::pair<ProcessState::States, ProcessState::States>(_old, _new));
      return;
    }

    c_lastcontrol = 0;
    uint32_t now = time(0);

    if ( _new == ProcessState::States::Maintenance ) {
      setPIN("buzzer", PINState::Off);
      setPIN("mtheat", PINState::Off);
      setPIN("mtpump", PINState::Off);
      setPIN("bkpump", PINState::Off);
    }

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

    if ( _new == ProcessState::States::Hopping ) {
      c_ps.setHoppingStart(now);
    }

    if ( _new == ProcessState::States::Cooling ) {
      setPIN("bkpump", PINState::On);
    }

    if ( _old == ProcessState::States::Cooling ) {
      setPIN("bkpump", PINState::Off);
    }

    if ( _new == ProcessState::States::Transfer ) {
      setPIN("buzzer", PINState::Off);
      setPIN("mtheat", PINState::Off);
      setPIN("mtpump", PINState::Off);
      setPIN("bkpump", PINState::On);
    }

    if ( _new == ProcessState::States::Finished ) {
      setPIN("buzzer", PINState::Off);
      setPIN("mtheat", PINState::Off);
      setPIN("mtpump", PINState::Off);
      setPIN("bkpump", PINState::Off);
    }

    if ( _new == ProcessState::States::Empty ) {
      c_prog = nullptr;
      c_heratiohistory.clear();
      setPIN("buzzer", PINState::Off);
      setPIN("mtheat", PINState::Off);
      setPIN("mtpump", PINState::Off);
      setPIN("bkpump", PINState::Off);
    }

    // when the state is reset
    if ( _old == ProcessState::States::Empty ||
	 _old == ProcessState::States::Maintenance ) {
      c_prog = nullptr;
      c_last_flow_volume = -1;
      c_temptarget = 0;
      c_heratiohistory.clear();
      setPIN("buzzer", PINState::Off);
      setPIN("mtheat", PINState::Off);
      setPIN("mtpump", PINState::Off);
      setPIN("bkpump", PINState::Off);
    }
  }

  void Controller::maintenanceMode(PINTracker &_pt) {
    bool pump = c_ps.getMaintPump();
    bool bkpump = c_ps.getMaintBKPump();
    bool heat = c_ps.getMaintHeat();
    float temp = c_ps.getMaintTemp();

#if 0
    printf("Controller::maintenanceMode P:%c H:%c T:%2.f\n",
	   pump?'t':'f',
	   heat?'t':'f',
	   temp);
#endif

    // controlling mtpump on heat is needed when the heating element
    // is directly on that circle, without an exchanger
#if 0
    setPIN("mtpump", ((pump || heat) ? PINState::On : PINState::Off));
#else
    setPIN("mtpump", (pump ? PINState::On : PINState::Off));
#endif
    setPIN("bkpump", (bkpump ? PINState::On : PINState::Off));

    setTempTarget(temp, 6.0f);
    c_needcontrol = heat;

  }

  void Controller::stageEmpty(PINTracker &_pt) {
    setPIN("mtheat", PINState::Off);
    setPIN("mtpump", PINState::Off);
    c_needcontrol = false;
  }

  void Controller::stageLoaded(PINTracker &_pt) {
    c_prog = c_ps.getProgram();
    // Loaded, so we should verify the timestamps
    uint32_t startat = c_ps.getStartat();
    c_hecycletime = c_cfg->getHECycleTime();

#if 0
    printf("Controller::stageLoaded(): state:%s\n", c_ps.getStringState().c_str());
#endif
    c_needcontrol = false;

    // set the cooling target
    c_ps.setCoolTemp(c_cfg->getCoolTemp());

    // in case of nomash, we're jumping to preboil
    if ( c_prog->getNoMash() ) {
      c_ps.setState(ProcessState::States::PreBoil);
      //printf("Controller::stageLoaded(): nomash detected, jumping to preboil\n");
      return;
    }

    // if we start immediately then jump to PreHeat
    if ( startat == 0 ) {
      c_ps.setState(ProcessState::States::PreHeat);
    } else {
      c_ps.setState(ProcessState::States::PreWait);
    }
    return;
  }

  void Controller::stagePreWait(PINTracker &_pt) {
    auto env = Environment::getInstance();
    float mttemp = env->getTempMT();
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

    c_log.trace("Controller::controllProcess() td:%.2f PreHeatTime:%u", tempdiff, phtime);
    uint32_t startat = c_ps.getStartat();
    if ( (now + phtime*1.15) > startat ) {
      c_ps.setState(ProcessState::States::PreHeat);
    }
    c_needcontrol = false;
  }

  void Controller::stagePreHeat(PINTracker &_pt) {
    auto env = Environment::getInstance();
    float mttemp = env->getTempMT();
    float rimstemp = env->getTempRIMS();
    float targettemp = c_prog->getStartTemp();
    //    float tempdiff = targettemp - mttemp;

    setTempTarget(targettemp, 6);
    if ( mttemp >= targettemp ) {
	c_ps.setState(ProcessState::States::NeedMalt);
    }
    c_needcontrol = true;
  }

  void Controller::stageNeedMalt(PINTracker &_pt) {
    // When the malts are added, temperature decreases
    // We have to heat it back up to the designated temperature
    float targettemp = c_prog->getStartTemp();
    setTempTarget(targettemp, c_cfg->getHeatOverhead());
    c_needcontrol = true;
  }

  void Controller::stageMashing(PINTracker &_pt) {
    const Program::MashSteps &steps = c_prog->getMashSteps();

    c_needcontrol = true;

    auto env = Environment::getInstance();
    int8_t msno = c_ps.getMashStep();
    float mttemp = env->getTempMT();
    int nsteps = steps.size();
    if ( msno >= nsteps ) {
      // we'll go to sparging here, but first we go up to endtemp
      float endtemp = c_prog->getEndTemp();

      setTempTarget(endtemp, c_cfg->getHeatOverhead()+6);
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
      setTempTarget(ms.temp, c_cfg->getHeatOverhead());

      if ( mttemp >= ms.temp - c_cfg->getTempAccuracy() ) {
	c_ps.setMashStep(0);
	c_ps.setMashStepStart(now);
	c_log.info("Controller::stageMashing(): starting step 0 at %.2f/%.2f", mttemp, ms.temp);
	return;
      }
    } else {
      // here we are doing the regular steps
      // heating up to the next step, is the responsibility of
      // the previous one, hence the -1 previously, heating up to the 0th step
      time_t start = c_ps.getMashStepStart();
      float target = ms.temp;
      float overhead = 0.8f;
      time_t held = now - start;

      // we held it for long enough, now we're aiming for the next step
      // and if we've reached it, we bump the step
      if ( held > ms.holdtime*60 ) {
	target = ((msno+1)<nsteps) ? steps[msno+1].temp : c_prog->getStartTemp();
	if ( mttemp + c_cfg->getTempAccuracy() > target) {
	  c_ps.setMashStep(msno+1);
	  c_ps.setMashStepStart(now);
#if 0
	  printf("Controller::stageMashing(): step %i->%i\n", msno, msno+1);
#endif
	} else {
	  overhead = c_cfg->getHeatOverhead();
	}
      }

      static time_t lastreport;
#if 0
      if ( lastreport != now && now % 5 == 0 ) {
	printf("Controller::stageMashing(): S:%hhi HT:%li/%i MT:%.2f T:%.2f\n",
	       msno, held, ms.holdtime, mttemp, target);
	lastreport = now;
      }
#endif
      setTempTarget(target, overhead);
    }
  }

  void Controller::stageSparging(PINTracker &_pt) {
    //printf("%s:%i:%s\n", __FILE__, __LINE__, __FUNCTION__);
    float endtemp = c_prog->getEndTemp();
    setTempTarget(endtemp, c_cfg->getHeatOverhead());
    c_needcontrol = true;
  }

  void Controller::stagePreBoil(PINTracker &_pt) {
    //printf("%s:%i:%s\n", __FILE__, __LINE__, __FUNCTION__);
    setPIN("mtpump", c_ps.getForceMTPump()?PINState::On : PINState::Off);
    setPIN("mtheat", PINState::Off);
    c_needcontrol = false;
  }

  void Controller::stageHopping(PINTracker &_pt) {
    //printf("%s:%i:%s\n", __FILE__, __LINE__, __FUNCTION__);
    setPIN("mtpump", c_ps.getForceMTPump()?PINState::On : PINState::Off);
    setPIN("mtheat", PINState::Off);
    c_needcontrol = false;

    auto prog = c_ps.getProgram();
    auto hops = prog->getHops();
    uint32_t now = time(0);

    uint32_t hopstart = c_ps.getHoppingStart();
    uint32_t boiltime = prog->getBoilTime();
    int32_t hoptime = hopstart + boiltime - now;
    c_ps.setHopTime(hoptime);

    // transition to the next stage when we're done
    if ( (hopstart + boiltime) < now ) {
      setPIN("buzzer", PINState::Off);
      c_ps.setState(ProcessState::States::Cooling);
      return;
    }

    for (auto &it: hops) {
      // we don't care with the past
      if ( it.attime >= hoptime ) {
	setPIN("buzzer", PINState::Off);
	continue;
      }

      int32_t tohop = hoptime - it.attime;
      c_ps.setHopId(it.id);
      if ( tohop > 180 ) {
	setPIN("buzzer", PINState::Off);
	break;
      }
      float onrate = 0.02f;
      float ctime = 3.0f;
      if ( tohop < 5 ) {
	onrate = 0.5f;
	ctime = 0.5f;
      } else if ( tohop < 15 ) {
	onrate = 0.4f;
	ctime = 3.0f;
      } else if ( tohop < 30 ) {
	onrate = 0.3f;
	ctime = 0.8f;
      } else if ( tohop < 60 ) {
	onrate = 0.1f;
	ctime = 1.0f;
      } else if ( tohop < 120 ) {
	onrate = 0.05f;
	ctime = 1.5f;
      } else {
	onrate = 0.025f;
	ctime = 3.0f;
      }

      setPIN("buzzer", PINState::Pulsate, ctime, onrate);
      break;
    }
  }

  void Controller::stageCooling(PINTracker &_pt) {
    float bktemp = c_ps.getSensorTemp("BK");

    setPIN("mtpump", c_ps.getForceMTPump()?PINState::On : PINState::Off);
    if ( bktemp <= c_ps.getCoolTemp() ) {
      //c_ps.setState(ProcessState::States::Finished);
      setPIN("buzzer", PINState::Pulsate, 1.0f, 0.23f);
    } else {
      setPIN("buzzer", PINState::Off);
    }
    c_needcontrol = false;
  }

  void Controller::stageTransfer(PINTracker &_pt) {
    setPIN("bkpump", c_ps.getBKPump()?PINState::On : PINState::Off);
    c_needcontrol = false;
  }

  void Controller::stageFinished(PINTracker &_pt) {
#if 0
    setPIN("buzzer", PINState::Off);
    setPIN("mtpump", PINState::Off);
    setPIN("bkpump", PINState::Off);
    setPIN("mtheat", PINState::Off);
#endif
    c_needcontrol = false;
  }


  void Controller::handleOutPIN(PINTracker::PIN &_pin) {
    c_mq_iocmd.send(PinStateMessage(_pin.getName(),
				    _pin.getNewValue(),
				    _pin.getNewCycletime(),
				    _pin.getNewOnratio()));
  }

  void Controller::setTempTarget(float _target, float _maxoverheat) {
    if ( c_temptarget != _target ||
	 c_tempoverheat != _maxoverheat )
      c_newtemptarget = true;

    c_temptarget = _target;
    c_tempoverheat = _maxoverheat;
  };

  int Controller::tempControl() {
    std::vector<int> lines;

    c_ps.setTargetTemp(c_temptarget);
    c_newtemptarget = false;

    if ( c_ps.getBlockHeat() ) {
      setPIN("mtheat", PINState::Off);
      setPIN("mtpump", PINState::On);
      return 1;
    }

    int nextcontrol = 30;
    TSDB& tsdb = c_ps.getThermoReadings();
    auto env = Environment::getInstance();

    time_t now = time(0);

    if ( getPIN("mtpump")->getOldValue() != PINState::On )
      setPIN("mtpump", PINState::On);

    float dt = now - c_lastcontrol;
    c_lastcontrol = now;

    float curr_rims(0), curr_mt(0), last_rims(0), last_mt(0);
    float dT_mt(0), dT_rims(0);
    bool nodata(false);

    // if we don't have enough data, then wait
    if ( !getTemps(ThermoCouple::RIMS, dt, last_rims, curr_rims, dT_rims) ||
	 !getTemps(ThermoCouple::MT, dt, last_mt, curr_mt, dT_mt) ) {
      setHERatio(c_hecycletime, 0);
      // if we don't have historical data, then simply use the last one
      curr_rims = env->getTempRIMS();
      curr_mt = env->getTempMT();
      nodata = true;
    }
    if ( curr_mt == 0.0f || curr_rims == 0.0f ) return 3;

    // set the RIMS tube target temperature
    float T_target_rims;
    if ( curr_mt >= c_temptarget ) {
      T_target_rims = c_temptarget * 1.01;
    } else {
      T_target_rims = c_temptarget + std::min(c_tempoverheat, 0.2f+(c_temptarget - curr_mt)*2.3f);
    }

    c_log.debug("Controller::tempControl(): %li RIMS: dt:%.2f last:%.2f curr:%.2f dT:%.4f RIMS_target:%.2f",
	   now, dt, last_rims, curr_rims, dT_rims, T_target_rims);
    c_log.debug("Controller::tempControl(): %li MT: dt:%.2f last:%.2f curr:%.2f dT:%.4f",
	   now, dt, last_mt, curr_mt, dT_mt);

    float last_ratio = getPIN("mtheat")->getOldOnratio();

    // First calculate how much power is needed to
    // heat up the whole stuff in the mashtun to the
    // target temperature

    float dT_mtc_temptarget = c_temptarget - curr_mt; // temp difference
    float dt_mt = dT_mtc_temptarget * 60.0; // heating with 1C/60s

    float hepwr = (1.0*c_cfg->getHEPower())/1000; // in kW
    float pwr_max = hepwr;
    float pwr_mt;
    float pwr_dissipation = 0;
    float pwr_dissipation_rims = 0;
    float pwr_abs_min = 0;
    float dTadjust = 1.2f; // FIXME should be moved to a config variable

    // when we're getting close to the target, but we don't have
    // data, then try to throttle the heating
    // this typically happens at Pre-Heating
    if ( dT_mtc_temptarget < dTadjust && nodata ) {
      float coeff_tgt = 0;
      float coeff_rt = 1;

      float halfpi = std::atan(INFINITY);
      float tgtdiff = c_temptarget*1.01 - curr_mt;
      float rimsdiff = curr_rims - c_temptarget;

      if ( tgtdiff > 0 ) {
	coeff_tgt = std::atan(tgtdiff)/halfpi;
	coeff_tgt = std::pow(coeff_tgt, 0.91);
      }
      if ( rimsdiff > 0 ) {
	coeff_rt = std::max(0.0f, std::atan(c_tempoverheat - rimsdiff)/halfpi);
	coeff_rt = std::pow(coeff_rt, 0.25);
      }
      float heratio = std::pow(coeff_tgt * coeff_rt, 0.71);

      c_log.debug("Controller::tempControl(): nodata heratio:%.2f", heratio);
      setHERatio(c_hecycletime, heratio);
      return 3;
    }

    // now calculate the MT heating requirements
    if ( nodata ) {
      // during preheat we don't have to care for 1C/60s
      if ( c_ps.getState() == ProcessState::States::PreHeat ||
	   c_ps.getState() == ProcessState::States::Maintenance ) {
	pwr_mt = hepwr;
      } else {
	// dT_mtc_temptarget is present on both sides of the division, that's why it's
	// omitted. This way the required power only depends on the volume -
	// as it's a constant heating
	pwr_mt = (4.2 * c_ps.getVolume() )/60;
      }
    } else {
      pwr_mt = (4.2 * c_ps.getVolume() )/60;
      // historical data is available here, so we use that to approximate
      // when we're hitting the target temperature

      if ( dT_mt > 0 ) {
	// when the temperature of the MashTun is increasing

	// check whether we're over the target temperature
	if ( dT_mtc_temptarget > 0 ) {
	  // linear approximation of the time it will take to
	  // heat it up with the current HERatio
	  float timetotarget = dT_mtc_temptarget / dT_mt;

	  if ( timetotarget < 45 ) {
	    pwr_mt *= 0.3;
	    nextcontrol = 5;
	  } else if ( timetotarget < 120 ) {
	    pwr_mt *= 0.6;
	    nextcontrol = 10;
	  }
	} else {
	  // if we're over the MT target temp, then stop heating
	  pwr_mt = 0;
	}
      } else {			// dT_mt < 0
	// when the MT is cooling, check whether we need to heat at all
	if ( dT_mtc_temptarget < 0 ) {
	  // it's fine here, we're already over the target temp
	  pwr_mt = 0;
	} else {
	  // pwr_mt's default should be good here, we're just decreasing the
	  // control time, if we're close to the target temp
	  if ( dT_mtc_temptarget < 1 ) {
	    nextcontrol = 10;
	  } else if ( dT_mtc_temptarget < 2 ) {
	    nextcontrol = 15;
	  }
	  pwr_mt = (4.2 * c_ps.getVolume() * dT_mtc_temptarget) / nextcontrol;

	  // adjust the max power. let's see how intensively we're cooling
	  {
	    int size = tsdb.size();
	    float mt300 = 0;
	    if ( size > 300 ) mt300 = tsdb.at(size-300)[ThermoCouple::MT];
	    else mt300 = tsdb.at(0)[ThermoCouple::MT];
	    if ( size<300 || mt300 < curr_mt) {
	      c_log.warn("!! RIMS cooling, can't find t-300 or it's cooler than current temp");
	      pwr_max = hepwr * 0.35;
	      c_log.warn("Limiting max power to 35%%: %.2f", pwr_max);
	    } else {
	      float diff_temp = mt300 - curr_mt;
	      uint32_t diff_time = size > 300 ? 300 : size;
	      float pwr_cooling = (4.2 * c_ps.getVolume() * diff_temp) / diff_time;

	      float currpwr = 3;

	      if ( c_heratiohistory.size()>0 ) {
		auto herd = c_heratiohistory[c_heratiohistory.size()-1];

		currpwr = herd.ratio * hepwr;
	      }

	      pwr_max = std::min(float(currpwr+(pwr_cooling*3)), hepwr);

	      c_log.warn("Cooling (%.2f C / %i sec) limiting pwr to %.2f",
			 diff_temp, diff_time, pwr_max);
	    }
	  }
	}
      }	// if ( dT_mt > 0 ) else
    }	// if ( nodata ) else

    c_log.debug("Controller::tempControl(): MT: dTc_temptarget:%.2f dt:%.2f hepwr:%.2f pwr_mt:%.2f nodata:%c",
		dT_mtc_temptarget, dt_mt, hepwr, pwr_mt,
		nodata ? 't' : 'f');

    // adjust the next control time
    if ( dt_mt < 60 ) {
      nextcontrol = std::max(10, int(dt/3));
    }

    // calculate the dissipation during the last cycle
    if ( !nodata ) {
      float pwr_last_effective = (4.2 * c_ps.getVolume() * (dT_mt*dt))/dt;
      float pwr_last = hepwr * last_ratio;
      float diff = pwr_last - pwr_last_effective;

      // we only apply dissipation if it's positive, and
      // it's value is less than 5% of the heating element
      // otherwise we're probably measuring lag.
      if ( diff > 0 && curr_rims > 50.0)
	pwr_dissipation = diff * ((curr_rims - 50)/40);

      if ( pwr_last_effective < 0 && (curr_mt + dt*30)<c_temptarget)
	pwr_abs_min = std::fabs(pwr_last_effective);
      c_log.debug("Controller::tempControl(): pwr last:%.2f eff:%.2f diff:%.5f",
		  pwr_last, pwr_last_effective, diff);
      c_log.debug("Controller::tempControl(): dissipation: %.3f pwr_abs_min:%.2f",
		  pwr_dissipation, pwr_abs_min);
    }

    /* The RIMS tube has a min a max value limiting its power,
     * and in between an actual calculated value
     * defaults are here, later we are limiting them
     */
    float pwr_rims_max = hepwr;
    float pwr_rims_min = hepwr * 0.02;
    float pwr_rims = hepwr;
    float pwr_rims_final;

    /*   ***********
     *   * R I M S *
     *   ***********/

    // without data, the RIMS tube is practically not adjustable
    if ( !nodata ) {
      float flowrate = calcFlowRate();
      c_log.trace("Flowrate: %.6f", flowrate);

      if ( flowrate > 0  ) {
	// again, time would be on both sides of the equation
	// V=flowrate*dt
	pwr_rims = (4.2 * flowrate * (T_target_rims - curr_mt))/1;
	if ( curr_rims < (T_target_rims - 1.5f) )
	  pwr_rims = (pwr_rims + hepwr)/2;

	c_log.info("pwr_rms(%.2f) = (4.2 * flowrate(%.5f) * (T_target_rims(%.2f) - curr_mt(%.2f)))/1",
		   pwr_rims, flowrate, T_target_rims, curr_mt);
      }

      if ( dT_mtc_temptarget < 0.5 ) {
	pwr_rims *= 0.83;
      } else if ( dT_mtc_temptarget < 1.0 ) {
	pwr_rims *= 0.89;
      } else if ( dT_mtc_temptarget < 2.0 ) {
	pwr_rims *= 0.95;
      }

      // set the max boundary
      if ( curr_mt > (c_temptarget + 0.5) ) {
	pwr_rims_max = 0;
      } else if ( curr_mt >= c_temptarget ) {
	// we are only compensating for the heat loss here
	pwr_rims_max = pwr_rims_min;
      }
      // if ( !nodata )
    } else {
      if ( curr_mt+2 < T_target_rims ) pwr_rims = pwr_rims_max;
    }

    // apply min/max boundaries
    // Controller::tempControl(): RIMS pwr: final:0.00 min:0.06 max:3.00 calc:1.93 max:0.00
    pwr_rims_final = std::min({std::max(pwr_rims_min, pwr_rims)+ pwr_dissipation_rims, pwr_rims_max, pwr_max});
    c_log.debug("Controller::tempControl(): RIMS pwr: final:%.2f min:%.2f max:%.2f calc:%.2f max:%.2f",
		pwr_rims_final, pwr_rims_min, pwr_rims_max, pwr_rims, pwr_max);
    float her_rims = pwr_rims_final / hepwr;

    // MashTun heating element on-ratio
    float her_mt = pwr_mt / hepwr;

    // here we also define a minimum power, because there's heat
    // dissipation around the tubing
    float pwr_final = std::max(pwr_abs_min, std::min({pwr_mt + pwr_dissipation, pwr_rims_final}));
    float heratio = pwr_final / hepwr;
    if ( heratio < 0.004f && curr_mt < c_temptarget && curr_rims < c_temptarget)
      heratio = 0.05f;
    c_log.info("Controller::tempControl(%.2f, %.2f): dT_rims:%.2f dT_MT_tgt:%.2f P_he:%.2f P_mt:%.2f P_rims:%.2f R:%.3f(MT:%.2f / RIMS:%.2f)",
		c_temptarget, c_tempoverheat,
		dT_rims, dT_mtc_temptarget,
		hepwr, pwr_mt, pwr_rims_final,
		heratio, her_mt, her_rims);

    setHERatio(c_hecycletime, heratio);
    return nextcontrol;
  }

  uint32_t Controller::calcHeatTime(uint32_t _vol, uint32_t _tempdiff, float _pkw) const {
    return (4.2 * _vol * _tempdiff)/_pkw;
  }

  bool Controller::getTemps(ThermoCouple _tc, uint32_t _dt, float &_last, float &_curr, float &_dT) {
    TSDB& db(c_ps.getThermoReadings());
    uint32_t size = db.size();
    if ( size <= _dt || _dt < 3 ) return false;
    try {
      ThermoReadings curr,last;
      _curr = db.last()[_tc];
      _last = db.at(size-1-_dt)[_tc];
    }
    catch (std::exception &e) {
      c_log.error("getTemps() failed %s", e.what());
      return false;
    }
    catch ( ... ) {
      c_log.error("getTemps() failed with unknown exception");
      return false;
    }
    _dT = (_curr - _last)/(1.0*_dt);
    return true;
  }

  void Controller::setHERatio(float _cycletime, float _ratio) {
    setPIN("mtheat", PINState::Pulsate, _cycletime, _ratio);
    c_heratiohistory.insert(_ratio);
  }

  /* Flow rate calculation
   * Formula: V = (P * dt) /(4.2 * dT)
   * V = volume
   * dt = time delta
   * P = power [kW]
   * dT = temperature delta
   */
  float Controller::calcFlowRate() {
    TSDB& db(c_ps.getThermoReadings());

    if ( db.size() < 30 ) return -1;
    uint32_t dbsize = db.size();

    uint32_t t_min_rims, t_min_mt;
    uint32_t startedat(0);
    uint32_t now = time(0);

    // started is the start time of the Mashing state
    startedat = c_ps.getStartat();
    // if it's 0, then we'll use the temphistory's
    // last timestamp
    if ( startedat <= 0 ) startedat = db.last().time;

    c_log.trace("calcFlowRate(): startedat: %u", startedat);


    uint32_t t_total(0);
    float V_total(0);
    uint32_t last_end = now - startedat;
    float hepwr = (1.0*c_cfg->getHEPower())/1000; // in kW

#if 0
    printf("now:%u startedat:%u last_end:%u\n", now, startedat, last_end);
#endif

    // loop on the heratio history backwards
    // MashTun
    for (ssize_t i=c_heratiohistory.size()-1; i>=0 && t_total < 180; --i) {
      auto heratiodata = c_heratiohistory[i];
      float heratio = heratiodata.ratio;
      uint32_t dt_start = std::abs(heratiodata.time - startedat);
      uint32_t dt_end = last_end;
      float T_start;
      float T_end;

      if ( dt_start >= db.size() ) continue;

#if 0
      printf("calcFlowRate()/dt_start:%u\n", dt_start);

      printf("calcFlowRate()/dbsize:&u - last_end:%u = %u",
	     dbsize, last_end, dbsize-last_end);
#endif
      auto tr = db.atDeltaTime(dbsize-last_end);

      auto tempsit = db.atDeltaTime(dt_start);
      if ( tempsit->dt != dt_start ) {
	c_log.trace("%s:%i: db.atDeltaTime(%u) miss: %lu",
		    __FILE__, __LINE__,
		    dt_start, tempsit->dt);
	last_end = dt_start;
	continue;
      }
      T_start = (*tempsit)[ThermoCouple::MT];
      T_end = (*tempsit)[ThermoCouple::RIMS];

      float dt = dt_end - dt_start;
      float pwr = hepwr * heratio;
      float dT = T_end - T_start;

      last_end = dt_start;
      // if the current cycle cooled down, skip it
      if ( dT < 0 ) continue;

      float V = (pwr * dt) / (4.2 * dT);

#if 0
      printf("Controller::calcFlowRate(): %u: %.4f = (%.2f * %.2f) / (4.2 * %.2f) (t_total:%u)\n",
	     it->first, V, pwr, dt, dT, t_total);
#endif

      if ( V <= 0 ) continue;

      V_total += V;
      t_total += dt;
    }

    float flowrate = V_total / t_total;
#if 0
    printf("Controller::calcFlowRate(): flowrate: %.4f = %.2f / %u\n",
	   flowrate, V_total, t_total);
#endif
    return flowrate;
  }
}
