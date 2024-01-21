
#include "FermdConfig.hh"

#include <iostream>
#include <fstream>

#include "common/logging.hh"
#include "common/Exception.hh"
#include "TiltDB.hh"
#include "MainLoop.hh"
#include "Bluetooth.hh"

namespace aegir {
  namespace fermd {

    /*
     * Config
     */
    FermdConfig::FermdConfig(): ConfigBase(),
				c_loglevel(blt::severity_level::info) {
      registerHandler("loglevel", c_loglevel);

      // pull in submodules
      registerHandler<TiltDB>("tilts");
      registerHandler<MainLoop>("threads");
      registerHandler<Bluetooth>("bluetooth");
    }

    FermdConfig::~FermdConfig() {
      save();
    }

    std::shared_ptr<FermdConfig> FermdConfig::getInstance() {
      static std::shared_ptr<FermdConfig> instance{new FermdConfig()};
      return instance;
    }

    FermdConfig &FermdConfig::setLogLevel(const std::string& _level) {
      static std::map<std::string, blt::severity_level> levelmap{
	{"trace", blt::severity_level::trace},
	{"debug", blt::severity_level::debug},
	{"info", blt::severity_level::info},
	{"warn", blt::severity_level::warning},
	{"warning", blt::severity_level::warning},
	{"error", blt::severity_level::error},
	{"fatal", blt::severity_level::fatal},
      };

      auto it = levelmap.find(_level);
      if ( it == levelmap.end() )
	throw Exception("Unknown loglevel %s", _level.c_str());

      return setLogLevel(it->second);
    }

    FermdConfig &FermdConfig::setLogLevel(blt::severity_level _level) {
      c_loglevel = _level;
      return *this;
    }
  }
}
