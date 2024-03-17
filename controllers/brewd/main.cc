#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <fcntl.h>
#include <sys/stat.h>

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

static uid_t getuser(const std::string &_name);
static uid_t getgroup(const std::string &_name);

int main(int argc, char *argv[]) {
  std::string cfgfile, pidfile, user, group;
  uid_t userid;
  gid_t groupid;
  bool initcfg(false), daemonize(false);

  po::options_description desc("Command line options");
  desc.add_options()
    ("help,h", "produce help message")
    ("config,c",po::value<std::string>(&cfgfile)->default_value("/usr/local/etc/aegir/brewd.yaml"),
     "Path to the config file")
    ("init-config,i", "Initialize a config with defaults and exit")
    ("daemonize,d", "daemonize")
    ("pidfile,p",po::value<std::string>(&pidfile)->default_value("/var/run/aegir-brewd.pid"),
     "Path to the PID file")
    ("user,u",po::value<std::string>(&user)->default_value("aegir"),
     "User to run as")
    ("group,g",po::value<std::string>(&group)->default_value("operator"),
     "Group to run as")
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

  aegir::logging::init();
  aegir::LogChannel log("main");
  //BOOST_LOG_TRIVIAL(info) << "Aegir starting up..";

  // parse the config file first
  auto cfg = aegir::Config::getInstance();
  if ( initcfg ) {
    try {
      cfg->save(cfgfile);
    }
    catch(aegir::Exception &e) {
      fprintf(stderr, "Error while saving configuration: %s\n", e.what());
      log.fatal("Error while saving configuration: %s", e.what());
      return 3;
    }
    printf("Initialized config file: %s\n", cfgfile.c_str());
    return 0;
  }
  // set the loglevel callback
  aegir::logging::setGetLogLevel([cfg]() {return cfg->getLogLevel();});
  // initcfg is done as well, normal operation from here
  log.log("Aegir starting up...");

  // looking up user, gid
  try {
    userid = getuser(user);
    log.debug("Translated user %s to %i", user.c_str(), userid);
    groupid = getgroup(group);
    log.debug("Translated group %s to %i", group.c_str(), groupid);
  }
  catch (aegir::Exception& e) {
    fprintf(stderr, "Error while looking up user and group: %s\n", e.what());
    log.fatal("Error while looking up user and group: %s", e.what());
    return 4;
  }

  // daemonize
  if ( daemonize ) {
    log.info("Daemonizing...");
    if ( daemon(0, 0)<0 ) {
      fprintf(stderr, "Error while calling daemon(): %s\n", strerror(errno));
      log.fatal("Error while calling daemon(): %s", strerror(errno));
      return 5;
    }

    pid_t fpid;

    if ( (fpid = fork())<0 ) {
      log.fatal("Error while calling fork(): %s", strerror(errno));
      return 6;
    } else {
      if ( fpid == 0 ) {
	log.info("Process %i stopping", getpid());
	return 0;
      }
    }
  }

  pid_t pid = getpid();
  log.info("Running under PID: %i", pid);

  // write the pid file
  {
    int fd;
    if ( (fd = open(pidfile.c_str(),
		    O_WRONLY|O_CREAT|O_TRUNC|O_SYNC,
		    S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH))>=0 ) {
      log.error("pid fd: %i", fd);
      if ( dprintf(fd, "%i\n", pid)<0 ) {
	log.error("Unable to write to pid file %s: %s",
		 pidfile.c_str(), strerror(errno));
	close(fd);
      } else {
	close(fd);
	log.info("Written PID file %s", pidfile.c_str());
      }
    } else {
      log.error("Unable to open pidfile %s: %s",
		pidfile.c_str(),
		strerror(errno));
    }
  }

  // change uid and gid
  if ( setgid(groupid)<0 ) {
    log.error("Unable to setgid(%i): %s", groupid, strerror(errno));
  }
  if ( setuid(userid)<0 ) {
    log.error("Unable to setuid(%i): %s", userid, strerror(errno));
  }

  // config file parsing
  try {
    cfg->load(cfgfile);
  }
  catch (aegir::Exception &e) {
    log.fatal("Error while loading config: %s", e.what());
    return 2;
  }

  // We have the config, now set GPIO up
  aegir::GPIO *gpio;

  try {
    gpio = aegir::GPIO::getInstance();
  }
  catch (aegir::Exception &e) {
    log.fatal("Error initializing GPIO interface: %s", e.what());
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
    log.fatal("Error starting up: %s", e.what());
    return 2;
  }
  catch (std::exception &e) {
    log.fatal("Error/std starting up: %s", e.what());
    return 2;
  }

  // deallocating stuff here
  delete gpio;

  log.info("Exitting...");
  return 0;
}

uid_t getuser(const std::string &_name) {
  struct passwd *pwd;

  if ( (pwd = getpwnam(_name.c_str())) == 0 )
    throw aegir::Exception("Unable to look up user: %s", strerror(errno));
  return pwd->pw_uid;
}

uid_t getgroup(const std::string &_name) {
  struct group *grp;

  if ( (grp = getgrnam(_name.c_str())) == 0 )
    throw aegir::Exception("Unable to look up user: %s", strerror(errno));
  return grp->gr_gid;
}

