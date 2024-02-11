/* config loader
 */

#ifndef AEGIR_FERMD_CONFIG_H
#define AEGIR_FERMD_CONFIG_H

#include "common/ConfigBase.hh"

namespace aegir {

  namespace fermd {
    // This is a base class for configuring
    // nodes in the config file.
    // Subsystems are providing their own
    // serialization to yaml here
    class FermdConfig: public aegir::ConfigBase {
    public:
      FermdConfig(const FermdConfig&) = delete;
      FermdConfig();
      ~FermdConfig();

      static std::shared_ptr<FermdConfig> getInstance();

      // getters
      inline const blt::severity_level getLogLevel() const { return c_loglevel; };

      // setters
      FermdConfig &setLogLevel(const std::string& _level);
      FermdConfig &setLogLevel(blt::severity_level _level);

    private:
      // the configfile
      std::string c_cfgfile;
      // loglevel
      blt::severity_level c_loglevel;
    };
    typedef std::shared_ptr<FermdConfig> fermdconfig_type;
  }
}

#endif
