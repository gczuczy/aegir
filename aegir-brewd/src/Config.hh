/*
  aegir-brewd config file reader and writer
  Config file is YAML, handled with yaml-cpp
  This should be a singleton
 */

#ifndef AEGIR_CONFIG_H
#define AEGIR_CONFIG_H

#include <string>
#include <map>
#include <set>

namespace aegir {
  class Config {
    Config() = delete;
    Config(const Config&) = delete;
    Config(Config&&) = delete;
    Config &operator=(const Config&) = delete;
    Config &operator=(Config&&) = delete;

  private:
    Config(const std::string &_cfgfile, bool _noload=false);
    void setDefaults();
    void load();
    void checkPinConfig(std::map<std::string, int> &_layout);

  private:
    static Config *c_instance;
    std::string c_cfgfile;
    // Config parameters
    // The device for the GPIO controller
    std::string c_device;
    // The PIN layout map
    std::map<std::string, int> c_pinlayout;
    std::set<std::string> c_pinnames;

  public:
    ~Config();
    static Config *instantiate(const std::string &_cfgfile, bool _noload=false);
    static Config *getInstance();
    Config &save();
  };
}

#endif
