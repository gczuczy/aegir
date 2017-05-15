#include "IOHandler.hh"

#include <stdio.h>
#include <sys/event.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>

#include <chrono>
#include <map>
#include <string>

#include "ZMQ.hh"
#include "GPIO.hh"
#include "Config.hh"

namespace aegir {

  IOHandler::IOHandler(GPIO &_gpio, SPI &_spi): c_gpio(_gpio), c_spi(_spi),
    c_mq_pub(ZMQ::SocketType::PUB),
    c_mq_iocmd(ZMQ::SocketType::SUB) {
    auto cfg = Config::getInstance();

    // first initialize the sensors
    for (int i=0; i<4; ++i) {
      c_tcs.push_back(std::make_unique<MAX31856>(c_spi, i));
    }
    // now set them up
    {
      auto nf = cfg->getMAX31856NoiseFilter();
      auto tctype = cfg->getMAX31856TCType();
      for (auto &it: c_tcs) {
	it->setAvgMode(MAX31856::AvgMode::S8); // averages of 4 samples
	it->set50Hz(nf == NoiseFilters::HZ50);
	it->setTCType(tctype);
	it->setConversionMode(true);
      }
    }
    // fetch the sensor mapping from the config
    cfg->getThermocouples(c_tcmap);
    // and the reading interval
    c_thermoival = cfg->getTCival();

    auto thrmgr = ThreadManager::getInstance();

    c_mq_pub.bind("inproc://iopub");
    c_mq_iocmd.bind("inproc://iocmd").subscribe("");

    thrmgr->addThread("IOHandler", *this);
  }

  IOHandler::~IOHandler() {
  }

  void IOHandler::readTCs() {
    struct timeval tv;
    gettimeofday(&tv, 0);
#ifdef AEGIR_DEBUG
    printf("\nIOHandler::readTCs():\n");
#endif
    for (auto &it: c_tcmap) {
      float temp = c_tcs[it.second]->readTCTemp();
      float cjtemp = c_tcs[it.second]->readCJTemp();
#ifdef AEGIR_DEBUG
      float cjto = c_tcs[it.second]->getCJOffset();
      printf("Sensor %s/%i temp: %f C Time: %li CJ:%.2f CJTO:%.4f\n", it.first.c_str(), it.second, temp,
	     tv.tv_sec, cjtemp, cjto);
#endif
      c_mq_pub.send(ThermoReadingMessage(it.first, temp, tv.tv_sec));
    }
  }

  void IOHandler::run() {
    printf("IOHandler started\n");

    std::map<std::string,int> inpins;
    std::set<std::string> outpins;

    // later we might need to handle unused pins for multiple configs here
    for ( auto &it: g_pinconfig ) {
      if ( it.second.mode == PinMode::IN ) {
#if AEGIR_DEBUG
	printf("IOHandle: Loading PIN %s\n", it.first.c_str());
#endif
	inpins[it.first] = -1;
      } else if ( it.second.mode == PinMode::OUT ) {
	outpins.insert(it.first);
      }
    }

    // set our kqueue up
    int kq;
    kq = kqueue();
    struct kevent ke[8];
    struct timespec ival;
    int nchanges = 8;
    int err;
    // EV_SET(kev, ident, filter, flags, fflags, data, udata);
    EV_SET(&ke[0], 0, EVFILT_TIMER, EV_ADD|EV_ENABLE, NOTE_SECONDS, c_thermoival, 0);
    if ( kevent(kq, ke, 1, 0, 0, 0) < 0 ) {
      printf("kevent failed: %i/%s\n", errno, strerror(errno));
    }

    int newval;
    std::shared_ptr<Message> msg;
    while ( c_run ) {
      // start with kevent
      ival.tv_sec = 0;
      ival.tv_nsec = 10000000;
      if ( (nchanges = kevent(kq, 0, 0, ke, 8, &ival)) > 0 ) {
	for (int i=0; i<nchanges; ++i) {
	  // EVFILT_TIMER with ident=0 is our sensor timer
	  // we only ready the sensors, when a brew process is active
	  if ( ke[i].filter == EVFILT_TIMER && ke[i].ident == 0 )
	    readTCs();
	}
      }

      // read the input pins
      for (auto &it: inpins) {
	newval = c_gpio[it.first].get();
	if ( newval != it.second ) {
#ifdef AEGIR_DEBUG
	  printf("IOHandle: Pin %s %i -> %i\n ", it.first.c_str(),
		 it.second, newval);
#endif
	  it.second = newval;
	  auto msg = PinStateMessage(it.first, newval);
	  c_mq_pub.send(msg);
	}
      }

      // check our input queue
      while ( (msg = c_mq_iocmd.recv()) != nullptr ) {
	if ( msg->type() == MessageType::PINSTATE ) {
	  auto psmsg = std::static_pointer_cast<PinStateMessage>(msg);
	  auto it = outpins.find(psmsg->getName());
	  if ( it != outpins.end() ) {
#ifdef AEGIR_DEBUG
	    printf("IOHandler: setting %s to %i\n", psmsg->getName().c_str(), psmsg->getState());
#endif
	    if ( psmsg->getState() == 1 ) {
	      c_gpio[psmsg->getName()].high();
	    } else {
	      c_gpio[psmsg->getName()].low();
	    }
	  } else {
	    printf("IOHandler: can't set %s to %i: no such pin\n", psmsg->getName().c_str(), psmsg->getState());
	  }
	}
      }
    }

    printf("IOHandler stopped\n");
  }
}
