#include <yaml-cpp/yaml.h>

#include <iostream>
#include <fstream>

#include "Config.hh"
#include "Exception.hh"

namespace aegir {
  Config *Config::c_instance=0;

  Config::Config(const std::string &_cfgfile, bool _noload): c_cfgfile(_cfgfile) {
    setDefaults();
    if ( !_noload ) load();
  }

  Config::~Config() {
  }

  Config *Config::instantiate(const std::string &_cfgfile, bool _noload) {
    if ( !c_instance ) c_instance = new Config(_cfgfile, _noload);
    return c_instance;
  }

  // TODO: throw exception when !c_instance
  Config *Config::getInstance() {
    return c_instance;
  }

  void Config::setDefaults() {
    c_device = "/dev/gpioctl0";

    // PIN layout
    c_pinlayout.clear();
    c_pinlayout["swled"] = 4;
    c_pinlayout["swon"]  = 17;
    c_pinlayout["swoff"] = 18;
    // and save the valid names
    c_pinnames.clear();
    for ( auto &it: c_pinlayout ) c_pinnames.insert(it.first);
  }

  void Config::load() {
    YAML::Node config;
    try {
      config = YAML::LoadFile(c_cfgfile);
    }
    catch (YAML::ParserException &e) {
      throw Exception("YAML::ParseException: %s", e.what());
    }
    catch (std::exception &e) {
      throw Exception("Unknown exception: %s", e.what());
    }
    catch (...) {
      throw Exception("Unknown exception");
    }

    // The config file is loaded into config, now put it into local variables
    try {
      if ( config["gpio"] && config["gpio"].IsMap() ) {
	YAML::Node gpio = config["gpio"];
	// load the device
	if ( gpio["device"] ) c_device = gpio["device"].as<std::string>();

	// the PIN configuration
	if ( gpio["pins"] && gpio["pins"].IsMap() ) {
	  YAML::Node pins = gpio["pins"];
	  std::map<std::string, int> pinlayout(c_pinlayout);
	  for (const auto &it: c_pinnames)
	    if ( pins[it] ) pinlayout[it] = pins[it].as<int>();

	  checkPinConfig(pinlayout);
	  c_pinlayout = pinlayout;
	} // PIN config
      } // GPIO section
    }
    catch (std::exception &e) {
      throw Exception("Error during parsing config: %s", e.what());
    }
  }

  void Config::checkPinConfig(std::map<std::string, int> &_layout) {
    std::map<int, int> count;

    for ( const auto &it: _layout ) {
      if ( count.find(it.second) == count.end() ) {
	count[it.second] = 0;
      } else {
	count[it.second] += 1;
      }
    }

    for ( const auto &it: count ) {
      if ( it.second > 1 ) throw Exception("Conflict on Pin %i\n", it.first);
    }
  }

  Config &Config::save() {
    YAML::Emitter yout;

    yout << YAML::BeginMap;
    yout << YAML::Key << "gpio";
    yout << YAML::Value << YAML::BeginMap;
    yout << YAML::Key << "device" << YAML::Value << c_device;
    yout << YAML::Key << "pins" << YAML::Value << c_pinlayout;
    yout << YAML::EndMap;
    yout << YAML::EndMap;

    std::ofstream outfile;

    outfile.open(c_cfgfile, std::ios::out | std::ios::trunc);
    outfile << yout.c_str() << std::endl;
    outfile.close();

    return *this;
  }
}
