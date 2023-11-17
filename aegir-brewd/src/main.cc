#include <stdio.h>

#include <string>
#include <sstream>

#include <boost/program_options.hpp>

#include "logging.hh"
#include "LogChannel.hh"
#include "Config.hh"
#include "Exception.hh"
#include "GPIO.hh"
#include "ThreadManager.hh"
#include "IOHandler.hh"
#include "Controller.hh"
#include "SPI.hh"
#include "DirectSelect.hh"
#include "PRThread.hh"

namespace po = boost::program_options;

int main(int argc, char *argv[]) {
  std::string cfgfile;
  bool initcfg(false);

  po::options_description desc("Command line options");
  desc.add_options()
    ("help,h", "produce help message")
    ("config,c", po::value<std::string>(&cfgfile)->default_value("/usr/local/etc/aegir-brewd.yaml"),
     "Path to config file")
    ("init-config,i", "Initialize a config with defaults and exit");

  po::variables_map vm;
  try {
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
  }
  catch (std::exception &e) {
    fprintf(stderr, "Exception while parsing arguments: %s\n", e.what());
    return 1;
  }
  catch (...) {
    fprintf(stderr, "Unknown exception while parsing arguments\n");
    return 1;
  }

  if ( vm.count("help") ) {
    std::stringbuf buf;
    std::ostream os(&buf);

    os << desc;

    printf("%s", buf.str().c_str());
    return 0;
  }

  if ( vm.count("init-config") ) initcfg = true;

  aegir::logging::init();
  aegir::LogChannel log("main");
  //BOOST_LOG_TRIVIAL(info) << "Aegir starting up..";
  log.log("Aegir starting up...");

  //printf("Config file: %s\n", cfgfile.c_str());

  // parse the config file first
  auto cfg = aegir::Config::getInstance();
  if ( initcfg ) {
    try {
      cfg->save(cfgfile);
    }
    catch(aegir::Exception &e) {
      fprintf(stderr, "Error while saving configuration: %s\n", e.what());
      return 3;
    }
    return 0;
  } else {
    try {
      cfg->load(cfgfile);
    }
    catch (aegir::Exception &e) {
      fprintf(stderr, "Error while loading config: %s\n", e.what());
      return 2;
    }
  }


  // We have the config, now set GPIO up
  aegir::GPIO *gpio;

  try {
    gpio = aegir::GPIO::getInstance();
  }
  catch (aegir::Exception &e) {
    fprintf(stderr, "Error initializing GPIO interface: %s\n", e.what());
    return 1;
  }

  // initialize the thread manager
  auto threadmgr = aegir::ThreadManager::getInstance();

  try {
    // Initialize the SPI bus
    std::map<int, std::string> dsmap{{0,"cs0"},{1,"cs1"},{2,"cs2"},{3,"cs3"}};
    aegir::DirectSelect ds(*gpio, dsmap);
    aegir::SPI spi(ds, *gpio, cfg->getSPIDevice());

    // init the worker thread objects
    auto ioh = new aegir::IOHandler(*gpio, spi);

    // init the controller
    auto ctrl = aegir::Controller::getInstance();

    // Init the PR thread
    aegir::PRThread prt;

    /// this is the main loop
    threadmgr->start();
    delete ioh;
  }
  catch (aegir::Exception &e) {
    fprintf(stderr, "Error starting up: %s\n", e.what());
    return 2;
  }
  catch (std::exception &e) {
    fprintf(stderr, "Error/std starting up: %s\n", e.what());
    return 2;
  }

  // deallocating stuff here
  delete gpio;

  return 0;
}
