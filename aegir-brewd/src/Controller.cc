#include "Controller.hh"

#include <map>
#include <set>
#include <string>

#include "Exception.hh"
#include "Config.hh"

namespace aegir {

  Controller *Controller::c_instance(0);

  Controller::Controller(): c_mq_io(ZMQ::SocketType::SUB) {
    // subscribe to our publisher for IO events
    try {
      c_mq_io.connect("inproc://iopub").subscribe("");
    }
    catch (std::exception &e) {
      printf("Sub failed: %s\n", e.what());
    }
    catch (...) {
      printf("Sub failed: unknown exception\n");
    }

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

    // Hold our states
    std::map<std::string, bool> inpinstate, inpinchanges, outpinqueue;
    std::set<std::string> outpins;
    for ( auto &it: g_pinconfig ) {
      if ( it.second.mode == PinMode::IN ) {
	inpinstate[it.first] = false;
      } else if ( it.second.mode == PinMode::OUT ) {
	outpins.insert(it.first);
      }
    }
    // check the initial list
    for ( auto &it: inpinstate ) {
      printf("%s: %i\n", it.first.c_str(), it.second?1:0);
    }

    // The main event loop
    std::shared_ptr<Message> msg;
    std::chrono::microseconds ival(10000);
    while ( c_run ) {
      inpinchanges = inpinstate;
      int nchanges = 0;

      // read the GPIO pin channel first
      try {
	while ( (msg = c_mq_io.recv()) != nullptr ) {
	  if ( msg->type() == MessageType::PINSTATE ) {
	    auto psmsg = std::static_pointer_cast<PinStateMessage>(msg);
	    printf("Controller: received %s:%i\n", psmsg->getName().c_str(), psmsg->getState());
	    auto it = inpinchanges.find(psmsg->getName());
	    if ( it == inpinchanges.end() ) {
	      printf("No such pin: '%s' %lu\n", psmsg->getName().c_str(), psmsg->getName().length());
	      continue;
	    }
	    it->second = psmsg->getState()==1;
	    //inpinchanges[psmsg->getName()] = psmsg->getState()==1;
	    ++nchanges;
	  } else {
	    printf("Got unhandled message type: %i\n", (int)msg->type());
	    continue;
	  }
	}
      }
      catch (Exception &e) {
	printf("Exception: %s\n", e.what());
      }
      // dump the state
      if ( 0 && nchanges ) {
	printf("Changes: %i\n", nchanges);
	for ( auto &it: inpinstate ) {
	  printf("%s: %i -> %i\n", it.first.c_str(),
		 it.second ? 1 : 0,
		 inpinchanges[it.first] ? 1 : 0);
	}
      }
      // handle the changes
      // off -> on
      if ( (inpinchanges["swon"] && !inpinchanges["swoff"]) &&
	   !(inpinstate["swon"] && !inpinstate["swoff"]) ) {
	printf("Sw changed from off -> on\n");
      } else if ( (!inpinchanges["swon"] && inpinchanges["swoff"]) &&
		  !(!inpinstate["swon"] && inpinstate["swoff"]) ) {
	printf("Sw changed from on -> off\n");
      }
      // save the new state
      inpinstate = inpinchanges;

      // sleep a bit
      std::this_thread::sleep_for(ival);
    }
  }
}
