#include "Controller.hh"

#include <time.h>
#include <sys/event.h>
#include <unistd.h>

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
			    c_mq_iocmd(ZMQ::SocketType::PUB), c_stoprecirc(false), c_needcontrol(false) {
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
      nevents = kevent(kq, 0, 0, kevents, 2, 0);

      // in case of errors, check again
      if ( nevents <= 0 ) continue;
      events.clear();
      for ( int i=0; i<nevents; ++i ) {
	events.insert(kevents[i].ident);
      }

      // PINTracker's cycle
      startCycle();

      // read the GPIO PINs first
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

      // state changes and controlling goes hand-in-hand
      ProcessState::States state, oldstate;
      state = c_ps.getState();
      do {
	oldstate = state;
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

	// the control event
	// also run it after the tempcontrol
	if ( events.find(kq_id_control) != events.end() ) {
	  // handle the current state
	  controlProcess(*this);
	}
	state = c_ps.getState();
      } while ( state != oldstate );

      // the temp control event
      if ( events.find(kq_id_temp) != events.end() ||
	   (c_needcontrol && c_newtemptarget)) {
	printf("Rnning tempcontrol...\n");
	nexttempcontrol = tempControl();
	printf("Tempcontrol said %i secs\n", nexttempcontrol);
	if ( nexttempcontrol < 3 ) nexttempcontrol = 3;
	else if ( nexttempcontrol > 30 ) nexttempcontrol = 30;
	tc_installed = false;	// oneshot fired, needs reinstall
      }

      // if we need tempcontrols, then keep on
      // installing the oneshot event
      if ( c_needcontrol && !tc_installed ) {
	//EV_SET(kev, ident, filter, flags, fflags, data, udata);
	EV_SET(&kevchanges[0], kq_id_temp, EVFILT_TIMER, EV_ADD|EV_ONESHOT, NOTE_SECONDS, nexttempcontrol, 0);

	// register the events
	kqerr = kevent(kq, kevchanges, 1, 0, 0, 0);
	printf("Installed tempcontrol for %i secs\n", nexttempcontrol);
	tc_installed = true;
      }

      // if we don't need tempcontrol and the event is installed, removeit
      if ( !c_needcontrol && tc_installed ) {
	//EV_SET(kev, ident, filter, flags, fflags, data, udata);
	EV_SET(&kevchanges[0], kq_id_temp, EVFILT_TIMER, EV_DELETE, 0, 0, 0);

	// register the events
	kqerr = kevent(kq, kevchanges, 1, 0, 0, 0);
	printf("Removed tempcontrol\n");
	tc_installed = false;
      }

      if ( c_stoprecirc ) {
	setPIN("rimspump", PINState::Off);
	setPIN("rimsheat", PINState::Off);
      } // stop recirculation

      // end the GPIO change cycle
      endCycle();
    } // while ( c_run )

    close(kq);

    printf("Controller stopped\n");
  }

  void Controller::reconfigure() {
    PINTracker::reconfigure();
    c_hecycletime = c_cfg->getHECycleTime();
    c_needcontrol = false;
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

    // calling the state function
    // if the state changes after, run the next state function as well
    auto it = c_stagehandlers.find(state);
    if ( it != c_stagehandlers.end() ) {
      it->second(this, _pt);
    } else {
      printf("No function found for state %hhu\n", state);
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

    // when the state is reset
    if ( _old == ProcessState::States::Empty ) {
      c_prog = nullptr;
      c_last_flow_volume = -1;
      setPIN("buzzer", PINState::Off);
      setPIN("rimsheat", PINState::Off);
      setPIN("rimspump", PINState::Off);
    }
  }

  void Controller::stageEmpty(PINTracker &_pt) {
    setPIN("rimsheat", PINState::Off);
    setPIN("rimspump", PINState::Off);
    c_needcontrol = false;
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
    c_needcontrol = false;
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
    c_needcontrol = false;
  }

  void Controller::stagePreHeat(PINTracker &_pt) {
    float mttemp = c_ps.getSensorTemp("MashTun");
    float rimstemp = c_ps.getSensorTemp("RIMS");
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

    int8_t msno = c_ps.getMashStep();
    float mttemp = c_ps.getSensorTemp("MashTun");
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
      setTempTarget(target, c_cfg->getHeatOverhead());
    }
  }

  void Controller::stageSparging(PINTracker &_pt) {
    printf("%s:%i:%s\n", __FILE__, __LINE__, __FUNCTION__);
    float endtemp = c_prog->getEndTemp();
    setTempTarget(endtemp, c_cfg->getHeatOverhead());
    c_needcontrol = true;
  }

  void Controller::stagePreBoil(PINTracker &_pt) {
    printf("%s:%i:%s\n", __FILE__, __LINE__, __FUNCTION__);
    c_needcontrol = false;
  }

  void Controller::stageHopping(PINTracker &_pt) {
    printf("%s:%i:%s\n", __FILE__, __LINE__, __FUNCTION__);
    c_needcontrol = false;
  }

  void Controller::stageCooling(PINTracker &_pt) {
    printf("%s:%i:%s\n", __FILE__, __LINE__, __FUNCTION__);
    c_needcontrol = false;
  }

  void Controller::stageFinished(PINTracker &_pt) {
    printf("%s:%i:%s\n", __FILE__, __LINE__, __FUNCTION__);
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
    float mttemp = c_ps.getSensorTemp("MashTun");
    float rimstemp = c_ps.getSensorTemp("RIMS");
    std::vector<int> lines;

    c_ps.setTargetTemp(c_temptarget);
    c_newtemptarget = false;

    if ( mttemp == 0.0f || rimstemp == 0.0f ) return 3;

    float tgtdiff = c_temptarget - mttemp;
    float rimsdiff = rimstemp - c_temptarget;
    int nextcontrol = 30;

    time_t now = time(0);

    if ( getPIN("rimspump")->getOldValue() != PINState::On )
      setPIN("rimspump", PINState::On);

    float dt = now - c_lastcontrol;
    c_lastcontrol = now;

    ProcessState::ThermoDataPoints temps_rims, temps_mt;
    c_ps.getTCReadings("RIMS", temps_rims);
    c_ps.getTCReadings("MashTun", temps_mt);

    float curr_rims(0), curr_mt(0), last_rims(0), last_mt(0);
    float dT_mt(0), dT_rims(0);
    bool nodata(false);

    // if we don't have enough data, then wait
    if ( !getTemps(temps_rims, dt, last_rims, curr_rims, dT_rims) ||
	 !getTemps(temps_mt, dt, last_mt, curr_mt, dT_mt) ) {
      setPIN("rimsheat", PINState::Off);
      // if we don't have historical data, then simply use the last one
      curr_rims = c_ps.getSensorTemp("RIMS");
      curr_mt = c_ps.getSensorTemp("MashTun");
      nodata = true;
    }
    printf("Controller::tempControl(): %li RIMS: dt:%.2f last:%.2f curr:%.2f dT:%.2f\n",
	   now, dt, last_rims, curr_rims, dT_rims);
    printf("Controller::tempControl(): %li MT: dt:%.2f last:%.2f curr:%.2f dT:%.2f\n",
	   now, dt, last_mt, curr_mt, dT_mt);

    float last_ratio = getPIN("rimsheat")->getOldOnratio();

    // First calculate how much power is needed to
    // heat up the whole stuff in the mashtun to the
    // target temperature

    float dT_mtc_temptarget = c_temptarget - curr_mt; // temp difference
    float dt_mt = dT_mtc_temptarget * 60.0; // heating with 1C/60s

    float hepwr = (1.0*c_cfg->getHEPower())/1000; // in kW
    float pwr_rims=hepwr; // declared here for debugging purposes
    float pwr_mt;
    float dTadjust = 1.2f; // FIXME should be moved to a config variable

    // when we're getting close to the target, but we don't have
    // data, then try to throttle the heating
    // this typically happens at Pre-Heating
    if ( dT_mtc_temptarget < dTadjust && nodata ) {
      float coeff_tgt = 0;
      float coeff_rt = 1;

      float halfpi = std::atan(INFINITY);
      float tgtdiff = c_temptarget - curr_mt;
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

      printf("Controller::tempControl(): nodata heratio:%.2f\n", heratio);
      setPIN("rimsheat", PINState::Pulsate, c_hecycletime, heratio);
      return 3;
    }

    // now calculate the MT heating requirements
    if ( nodata ) {
      // during preheat we don't have to care for 1C/60s
      if ( c_ps.getState() == ProcessState::States::PreHeat ) {
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

	  if ( timetotarget < 10 ) {
	    pwr_mt *= 0.3;
	    nextcontrol = 5;
	  } else if ( timetotarget < 30 ) {
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
	    nextcontrol = 5;
	  } else if ( dT_mtc_temptarget < 2 ) {
	    nextcontrol = 10;
	  }
	}
      }	// if ( dT_mt > 0 ) else
    }	// if ( nodata ) else

    printf("Controller::tempControl(): MT: dTc_temptarget:%.2f dt:%.2f hepwr:%.2f pwr_mt:%.2f nodata:%c\n",
	   dT_mtc_temptarget, dt_mt, hepwr, pwr_mt,
	   nodata ? 't' : 'f');

    // adjust the next control time
    if ( dt_mt < 60 ) {
      nextcontrol = dt/3;
    }

    // heating element on-rations
    float her_mt = pwr_mt / hepwr;
    float her_rims = 1; // this will be capped down by min() with her_mt

    // if the rimstube wasn't cooling, then
    // we can calculate the flowrate from the heating
    // and how much do we need to heat it up
    if ( dT_rims > 0 && !nodata ) {
      // when we're not reaching the mashtun target within a minute,
      // then it's sufficient to keep the tube at c_temptarget+c_tempoverheat
      float target_rims = c_temptarget + c_tempoverheat;

      // first calculate the water flow
      float volume = (1.0*dt * hepwr * last_ratio)/(4.2 * (curr_rims - last_mt));
      c_last_flow_volume = volume/dt;

      // calculate the needed power
      pwr_rims = (4.2 * volume * (target_rims - curr_mt))/nextcontrol;

      // adjust the next control, to avoid rims temp overruns
      float dt_rims_target = target_rims / dT_rims;
      nextcontrol = std::min(nextcontrol, int (dt_rims_target / 3));
    } else if ( dT_rims < 0 ) {
      // if the RIMS tube is cooling, then check whether we're over the
      // temperature overhead
      float target_rims = c_temptarget + c_tempoverheat;
      float dT_rims_target = target_rims - curr_rims;

      printf("RIMS cooling LFV:%.4f\n", c_last_flow_volume);

      if ( dT_rims_target > 3 ) {
	pwr_rims = hepwr;
      } else {
	// calculate the needed power based on the previous flow rate
	pwr_rims = (4.2 * c_last_flow_volume*nextcontrol * (target_rims - curr_mt))/nextcontrol;
	pwr_rims *= 0.9; // and undercut it by 10%
      }
    }

    // if the MT has reached the target, and
    // the RIMS tube is cooling, try to keep it on temp
    float pwr_rims_min = 0;
    if ( curr_mt >= c_temptarget &&
	 curr_rims <= c_temptarget - 0.2 ) {
      pwr_rims_min = (4.2 * c_last_flow_volume*nextcontrol * (c_temptarget - curr_rims)) / nextcontrol;

      // this can't be more than 10% of the total HE power
      pwr_rims_min = std::min(pwr_rims_min, hepwr/10);
    }

    // calculate the HE ratio
    // FIXME: at the final heratio this needs to be applied conditionallyxs
    if ( pwr_rims_min > 0 )
      pwr_rims = std::max(pwr_rims, pwr_rims_min);
    her_rims = pwr_rims / hepwr;

    float heratio = std::min(her_mt, her_rims);
    printf("Controller::tempControl(%.2f, %.2f): dT_rims:%.2f dT_MT_tgt:%.2f P_he:%.2f P_mt:%.2f P_rims:%.2f R:%.2f(MT:%.2f / RIMS:%.2f)\n",
	   c_temptarget, c_tempoverheat,
	   dT_rims, dT_mtc_temptarget,
	   hepwr, pwr_mt, pwr_rims,
	   heratio, her_mt, her_rims);

    setPIN("rimsheat", PINState::Pulsate, c_hecycletime, heratio);
    return nextcontrol;
  }

  uint32_t Controller::calcHeatTime(uint32_t _vol, uint32_t _tempdiff, float _pkw) const {
    return (4.2 * _vol * _tempdiff)/_pkw;
  }

  bool Controller::getTemps(const ProcessState::ThermoDataPoints &_tdp, int _dt, float &_last, float &_curr, float &_dT) {
    int size = _tdp.size();
    if ( size < _dt ) return false;
    try {
      _curr = _tdp.at(size-1);
      _last = _tdp.at(size-1-_dt);
    }
    catch (...) {
      return false;
    }
    _dT = (_curr - _last)/(1.0*_dt);
    return true;
  }
}
