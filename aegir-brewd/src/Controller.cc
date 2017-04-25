#include "Controller.hh"

#include <map>
#include <set>
#include <string>

#include "Exception.hh"
#include "ProcessState.hh"

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
	      setPIN(psmsg->getName(), psmsg->getState()==1);
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
	    ps.addThermoReading(trmsg->getName(), trmsg->getTimestamp(), trmsg->getTimestamp());
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

    // The pump&heat control button
    if ( _pt.hasChanges() ) {
      std::shared_ptr<PINTracker::PIN> swon(_pt.getPIN("swon"));
      std::shared_ptr<PINTracker::PIN> swoff(_pt.getPIN("swoff"));
      // whether we have at least one of the controls
      if ( swon->isChanged() || swoff->isChanged() ) {
	if ( swon->getNewValue() && !swoff->getNewValue() ) {
	  setPIN("swled", true);
	} else if ( !swon->getNewValue() && swoff->getNewValue() ) {
	  setPIN("swled", false);
	}
      }
    } // pump&heat switch
  }

  void Controller::handleOutPIN(PINTracker::PIN &_pin) {
    c_mq_iocmd.send(PinStateMessage(_pin.getName(), _pin.getNewValue()?1:0));
  }
}
