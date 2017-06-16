#include "Controller.hh"

#include <time.h>

#include <map>
#include <set>
#include <string>

#include "Exception.hh"
#include "ProcessState.hh"
#include "Config.hh"

namespace aegir {
  /*
   * Controller
   */

  Controller *Controller::c_instance(0);

  Controller::Controller(): PINTracker(), c_mq_io(ZMQ::SocketType::SUB),
			    c_mq_iocmd(ZMQ::SocketType::PUB) {
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

    // reconfigure state variables
    reconfigure();

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

    // the process states
    ProcessState &ps(ProcessState::getInstance());

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
	    printf("Controller: received %s:%i\n", psmsg->getName().c_str(), psmsg->getState());
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
	    ps.addThermoReading(trmsg->getName(), trmsg->getTimestamp(), trmsg->getTemp());
	  } else {
	    printf("Got unhandled message type: %i\n", (int)msg->type());
	    continue;
	  }
	}
      } // End of pin and sensor readings
      catch (Exception &e) {
	printf("Exception: %s\n", e.what());
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
  }

  void Controller::controlProcess(PINTracker &_pt) {
    ProcessState &ps(ProcessState::getInstance());
    ProcessState::Guard guard_ps(ps);
    ProcessState::States state = ps.getState();
    auto prog = ps.getProgram();

    // The pump&heat control button
    if ( _pt.hasChanges() ) {
      std::shared_ptr<PINTracker::PIN> swon(_pt.getPIN("swon"));
      std::shared_ptr<PINTracker::PIN> swoff(_pt.getPIN("swoff"));
      // whether we have at least one of the controls
      if ( swon->isChanged() || swoff->isChanged() ) {
	if ( swon->getNewValue() != PINState::Off
	     && swoff->getNewValue() == PINState::Off ) {
	  setPIN("swled", PINState::On);
	} else if ( swon->getNewValue() == PINState::Off &&
		    swoff->getNewValue() != PINState::Off ) {
	  setPIN("swled", PINState::Off);
	}
      }
    } // pump&heat switch

    if ( state == ProcessState::States::Loaded ) {
      // Loaded, so we should verify the timestamps
      uint32_t startat = ps.getStartat();
      // if we start immediately then jump to PreHeat
      if ( startat == 0 ) {
	ps.setState(ProcessState::States::PreHeat);
      } else {
	ps.setState(ProcessState::States::PreWait);
      }
      return;
    } // Loaded

    if ( state == ProcessState::States::PreWait ) {
      float mttemp = ps.getSensorTemp("MashTun");
      if ( mttemp == 0 ) return;
      // let's see how much time do we have till we have to start pre-heating
      uint32_t now = time(0);
      // calculate how much time
      Config *cfg = Config::getInstance();
      float tempdiff = prog->getStartTemp() - mttemp;
      uint32_t phtime = 0;
      if ( tempdiff > 0 )
	phtime = calcHeatTime(ps.getVolume(), tempdiff, 0.001*cfg->getHEPower());

      printf("Controller::controllProcess() td:%.2f PreHeatTime:%u\n", tempdiff, phtime);
      uint32_t startat = ps.getStartat();
      if ( (now + phtime*1.15) > startat )
	ps.setState(ProcessState::States::PreHeat);
      else
	return;
    } // PreWait

    if ( state == ProcessState::States::PreHeat ) {
    }

  }

  void Controller::handleOutPIN(PINTracker::PIN &_pin) {
    c_mq_iocmd.send(PinStateMessage(_pin.getName(),
				    _pin.getNewValue(),
				    _pin.getNewCycletime(),
				    _pin.getNewOnratio()));
  }

  uint32_t Controller::calcHeatTime(uint32_t _vol, uint32_t _tempdiff, float _pkw) const {
    return (4.2 * _vol * _tempdiff)/_pkw;
  }
}
