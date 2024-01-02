#include "IOHandler.hh"

#include <stdio.h>
#include <sys/event.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#ifdef AEGIR_DEBUG
#include <chrono>
#endif
#include <regex>

#include "ZMQ.hh"
#include "GPIO.hh"
#include "Config.hh"
#include "Environment.hh"

#define KE_LEN 32
// the oneshot offset for kqueue ident
#define ID_OS_OFFSET 100
// the pulse offset for kqueue ident
#define ID_PULSE_OFFSET 1000

namespace aegir {

  IOHandler::IOHandler(GPIO &_gpio, SPI &_spi): c_gpio(_gpio), c_spi(_spi),
						c_mq_pub(ZMQ::SocketType::PUB),
						c_mq_iocmd(ZMQ::SocketType::SUB),
						c_log("IOHandler") {
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
    c_tcmap = cfg->getThermocouples();
    // and the reading interval
    c_thermoival = cfg->getTCival();
    // and the pin polling ival
    c_pinival = cfg->getPINival();

    auto thrmgr = ThreadManager::getInstance();

    c_mq_pub.bind("inproc://iopub");
    c_mq_iocmd.bind("inproc://iocmd").subscribe("");

    // later we might need to handle unused pins for multiple configs here
    for ( auto &it: g_pinconfig ) {
      if ( it.second.mode == PinMode::IN ) {
#ifdef AEGIR_DEBUG
	printf("IOHandle: Loading PIN %s\n", it.first.c_str());
#endif
	c_inpins[it.first] = PINState::Unknown;
      } else if ( it.second.mode == PinMode::OUT ) {
	c_outpins[it.first].state = PINState::Off;
	c_outpins[it.first].name = it.first;
	c_gpio[it.first].low();
      }
    }

    c_kq = kqueue();

    thrmgr->addThread("IOHandler", *this);
  }

  IOHandler::~IOHandler() {
    std::regex re_cs("^cs[0-9]$");
    std::smatch m;
    close(c_kq);
    for ( auto &it: c_outpins ) {
      if ( std::regex_match(it.first, m, re_cs) ) {
	c_gpio[it.first].high();
      } else {
	c_gpio[it.first].low();
      }
    }
  }

  void IOHandler::readTCs() {
    struct timeval tv;
    gettimeofday(&tv, 0);
    ThermoReadings tr;
    for (int i=0; i < ThermoCouple::_SIZE; ++i) {
      tr[i] = c_tcs[c_tcmap.tcs[i]]->readTCTemp();
    }
    try {
      c_mq_pub.send(ThermoReadingMessage(tr, tv.tv_sec));
    }
    catch (Exception &e) {
      c_log.error("IOHandler::readTCs zmq send failed: %s", e.what());
    }
    try {
      Environment::getInstance()->setThermoReadings(tr);
    }
    catch (Exception &e) {
      c_log.error("IOHandler::readTCs Environment::setThermoReadings failed: %s", e.what());
    }
  }

  void IOHandler::handlePins() {
    PINState newval;
    std::shared_ptr<Message> msg;

    // read the input pins
    for (auto &it: c_inpins) {
      newval = c_gpio[it.first].get();
      if ( newval != it.second ) {
#ifdef AEGIR_DEBUG
	printf("IOHandle: Pin %s %hhu -> %hhu\n ", it.first.c_str(),
	       (uint8_t)it.second, (uint8_t)newval);
#endif
	it.second = newval;
	auto msg = PinStateMessage(it.first, newval);
	try {
	  c_mq_pub.send(msg);
	}
	catch (Exception &e) {
	  c_log.error("IOHandler::handlePins() zmq send failure: %s", e.what());
	}
      }
    }

    // check our input queue
    while ( (msg = c_mq_iocmd.recv()) != nullptr ) {
      if ( msg->type() == MessageType::PINSTATE ) {
	auto psmsg = std::static_pointer_cast<PinStateMessage>(msg);
	auto it = c_outpins.find(psmsg->getName());
	if ( it != c_outpins.end() ) {
#if defined(AEGIR_DEBUG)
	  printf("IOHandler: setting %s to %hhu\n", psmsg->getName().c_str(), (uint8_t)psmsg->getState());
#endif
	  // always clear the pulsate state, it's either not needed,
	  // or has to be replaced by new parameters
	  if ( c_outpins[psmsg->getName()].state == PINState::Pulsate ) {
	    clearPulsate(c_gpio[psmsg->getName()].getID());
	    c_gpio[psmsg->getName()].low();
	  }

	  if ( psmsg->getState() == PINState::On ) {
	    c_gpio[psmsg->getName()].high();
	    c_outpins[psmsg->getName()].state = PINState::On;

	  } else if ( psmsg->getState() == PINState::Off )  {
	    c_gpio[psmsg->getName()].low();
	    c_outpins[psmsg->getName()].state = PINState::Off;

	  } else if ( psmsg->getState() == PINState::Pulsate ) {
	    // for pulsating, we have to start with an On state, then
	    // create a oneshot for adding the repeating Off
	    struct kevent ke[2];
	    // cycle time in milliseconds
	    int ctms = 1000*psmsg->getCycletime();
	    // offset in milliseconds
	    int offsetms = ctms*psmsg->getOnratio();
	    // the gpio pin id
	    int id = c_gpio[psmsg->getName()].getID();
	    // the passed udata
	    outpindata *opd = &(c_outpins[psmsg->getName()]);
	    void *udata = (void*)opd;
	    opd->cycletime = ctms;
	    opd->onratio   = psmsg->getOnratio();

#ifdef AEGIR_DEBUG
	    printf("IOHandler starting pulsate: %s/%i C:%i OR:%.2f\n", psmsg->getName().c_str(), id,
		   opd->cycletime, opd->onratio);
#endif

	    // EV_SET(kev, ident, filter, flags, fflags, data, udata);
#ifdef AEGIR_DEBUG
	    printf("IOHandler EV_SET(%i, TIMER, ADD|ENABLE, MSECONDS, %i, udata)\n",
		   ID_PULSE_OFFSET+2*id+1, ctms);
	    printf("IOHandler EV_SET(%i, TIMER, ADD|ENABLE|ONESHOT, MSECONDS, %i, udata)\n",
		   ID_OS_OFFSET+id, offsetms);
#endif
	    EV_SET(&ke[0], ID_PULSE_OFFSET+2*id+1, EVFILT_TIMER, EV_ADD|EV_ENABLE, NOTE_MSECONDS, ctms, udata);
	    EV_SET(&ke[1], ID_OS_OFFSET+id, EVFILT_TIMER, EV_ADD|EV_ENABLE|EV_ONESHOT, NOTE_MSECONDS, offsetms, udata);
	    if ( kevent(c_kq, ke, 2, 0, 0, 0) < 0 ) {
	      c_log.error("kevent failed: %i/%s\n", errno, strerror(errno));
	    }
	    c_gpio[psmsg->getName()].high();
	    c_outpins[psmsg->getName()].state = PINState::Pulsate;
	  } else {
	    c_log.error("IOHandler:%i: Unhandled pinstate %hhu", __LINE__, psmsg->getState());
	  }
	} else {
	  c_log.error("IOHandler: can't set %s to %hhu: no such pin", psmsg->getName().c_str(), psmsg->getState());
	}
      }
    }
  }

  void IOHandler::clearPulsate(int id) {
    struct kevent ke[3];

    // EV_SET(kev, ident, filter, flags, fflags, data, udata);
    EV_SET(&ke[0], ID_PULSE_OFFSET+2*id+0, EVFILT_TIMER, EV_DELETE, 0, 0, 0);
    EV_SET(&ke[1], ID_PULSE_OFFSET+2*id+1, EVFILT_TIMER, EV_DELETE, 0, 0, 0);
    EV_SET(&ke[2], ID_OS_OFFSET+id, EVFILT_TIMER, EV_DELETE, 0, 0, 0);
    kevent(c_kq, ke, 3, 0, 0, 0);
  }

  void IOHandler::run() {
    c_log.info("IOHandler started");

    // set our kqueue up
    struct kevent ke[KE_LEN];
    struct kevent oske;
    int nchanges = KE_LEN;
    int ident;
    int filter;
    // EV_SET(kev, ident, filter, flags, fflags, data, udata);
    EV_SET(&ke[0], 0, EVFILT_TIMER, EV_ADD|EV_ENABLE, NOTE_SECONDS, c_thermoival, 0);
    EV_SET(&ke[1], 1, EVFILT_TIMER, EV_ADD|EV_ENABLE, NOTE_MSECONDS, c_pinival, 0);
    if ( kevent(c_kq, ke, 2, 0, 0, 0) < 0 ) {
      c_log.error("kevent failed: %i/%s", errno, strerror(errno));
    }

    while ( c_run ) {
      // start with kevent
      if ( (nchanges = kevent(c_kq, 0, 0, ke, KE_LEN, 0)) > 0 ) {
	for (int i=0; i<nchanges; ++i) {
	  ident  = ke[i].ident;
	  filter = ke[i].filter;
	  // EVFILT_TIMER with ident=0 is our sensor timer
	  // we only read the sensors, when a brew process is active
	  if ( filter == EVFILT_TIMER && ident == 0 ) {
	    // TC reading
	    readTCs();
	  } else if ( filter == EVFILT_TIMER && ident == 1 ) {
	    // general PIN handling
	    handlePins();
	  } else if ( filter == EVFILT_TIMER && ident >= ID_OS_OFFSET && ident < ID_PULSE_OFFSET ) {
	    // offset timer installation
	    int pindent = ident - ID_OS_OFFSET;
	    outpindata *opd = (outpindata*)ke[i].udata;

	    EV_SET(&oske, ID_PULSE_OFFSET+2*pindent+0, EVFILT_TIMER, EV_ADD|EV_ENABLE, NOTE_MSECONDS, opd->cycletime, (void*)opd);
	    c_gpio[opd->name].low();
	    if ( kevent(c_kq, &oske, 1, 0, 0, 0) < 0 ) {
	      c_log.error("kevent failed: %i/%s", errno, strerror(errno));
	    }

	  } else if ( filter == EVFILT_TIMER && ident >= ID_PULSE_OFFSET ) {
	    int pindent = ((ident - ID_PULSE_OFFSET) & 0x0ffe)/2;
	    int up = ident & 1;
	    outpindata *opd = (outpindata*)ke[i].udata;

#ifdef AEGIR_DEBUG
	    printf("IOHandler timer ident %i -> %i up:%i\n", ident, pindent, up);
#endif

	    if ( up ) {
	      c_gpio[opd->name].high();
	    } else {
	      c_gpio[opd->name].low();
	    }
	  }
	}
      }

    }

    c_mq_pub.close();
    c_mq_iocmd.close();
    // these are in the dtor
    //c_gpio;
    //c_spi;
    c_log.info("IOHandler stopped");
  }
}
