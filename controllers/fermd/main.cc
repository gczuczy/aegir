#include <stdio.h>

#include <string>

#include <boost/program_options.hpp>

#include "common/logging.hh"
#include "common/LogChannel.hh"
#include "common/ryml.hh"
#include "FermdConfig.hh"
#include "common/Exception.hh"
#include "MainLoop.hh"
#include "ServiceManager.hh"

namespace po = boost::program_options;

int main(int argc, char* argv[]) {
  std::string cfgfile, pidfile;
  bool initcfg(false), daemonize(false);

  po::options_description desc("Command line options");
  desc.add_options()
    ("help,h", "produce help message")
    ("config,c",po::value<std::string>(&cfgfile)->default_value("/usr/local/etc/aegir/fermd.yaml"),
     "Path to the config file")
    ("init-config,i", "Initialize a config with defaults and exit")
    ("daemonize,d", "daemonize")
    ("pidfile,p",po::value<std::string>(&pidfile)->default_value("/var/run/aegir-brewd.pid"),
     "Path to the PID file")
      ;

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
  if ( vm.count("daemonize") ) daemonize = true;

  aegir::init_ryml();

  // initialize logging
  aegir::logging::init();
  aegir::LogChannel log("main");

  // config first
  aegir::fermd::fermdconfig_type cfg;
  std::shared_ptr<aegir::fermd::ServiceManager> sm;
  try {
    sm = std::make_shared<aegir::fermd::ServiceManager>();
    cfg = sm->get<aegir::fermd::FermdConfig>();
  }
  catch (aegir::Exception& e) {
    printf("Fatal error: %s\n", e.what());
    return 1;
  }
  catch (std::exception& e) {
    printf("Fatal error: %s\n", e.what());
    return 2;
  }
  catch (...) {
    printf("Unknown fatal error\n");
    return 3;
  }
  // set the loglevel callback
  //aegir::logging::setGetLogLevel([cfg]() {return cfg->getLogLevel();});

  if ( initcfg ) {
    try {
      cfg->save(cfgfile);
    }
    catch (aegir::Exception& e) {
      printf("Error while initializong config file: %s\n", e.what());
      return 1;
    }
    catch (std::exception& e) {
      printf("Error while initializong config file: %s\n", e.what());
      return 1;
    }
    catch (...) {
      printf("Unknown error while initializong config file\n");
      return 1;
    }
    return 0;
  }

  //daemonization

  // the main loop
  try {
    auto ml = sm->get<aegir::fermd::MainLoop>();
    ml->run();
  }
  catch (aegir::Exception& e) {
    printf("Fatal error: %s\n", e.what());
    return 1;
  }
  catch (std::exception& e) {
    printf("Fatal error: %s\n", e.what());
    return 2;
  }
  catch (...) {
    printf("Unknown fatal error\n");
    return 3;
  }

  return 0;
}

