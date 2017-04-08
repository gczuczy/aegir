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

#include "MAX31856.hh"

namespace aegir {

  using pinlayout_t = std::map<std::string, int>;
  enum class PinMode { IN, OUT };
  enum class PinPull { NONE, DOWN, UP};
  enum class ChipSelectors {DirectSelect};
  enum class NoiseFilters {HZ50, HZ60};
  // we will have to add default states for out pins
  struct PinConfig {
    PinConfig() {};
    PinConfig(PinMode _pm, PinPull _pp): mode(_pm), pull(_pp) {};
    PinMode mode;
    PinPull pull;
  };

  using pinconfig_t = std::map<std::string, PinConfig>;

  extern pinconfig_t g_pinconfig;

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
    pinlayout_t c_pinlayout;
    // SPI config
    std::string c_spidev;
    ChipSelectors c_spi_chipselector;
    // SPI / MAX31856
    MAX31856::TCType c_spi_max31856_tctype;
    NoiseFilters c_spi_max31856_noisefilter;
    // SPI / DirectSelect pin layout
    std::map<int, std::string> c_spi_dschips;
    // thermocouple layout
    std::map<std::string, int> c_thermocouples;
    // thermocouple reading interval in seconds
    uint32_t c_thermoival;
    // PR ZMQ address
    uint16_t c_zmq_pr_port;

  public:
    ~Config();
    static Config *instantiate(const std::string &_cfgfile, bool _noload=false);
    static Config *getInstance();
    Config &save();

    // retrieve config elements
    const std::string &getGPIODevice() const;
    Config &getPinConfig(pinlayout_t &_layout);
    inline const std::string &getSPIDevice() const {return c_spidev;};
    inline const ChipSelectors getSPIChipSelector() const {return c_spi_chipselector;};
    inline const MAX31856::TCType getMAX31856TCType() const {return c_spi_max31856_tctype;};
    inline const NoiseFilters getMAX31856NoiseFilter() const { return c_spi_max31856_noisefilter;};
    inline const void getSPIDSChips(std::map<int, std::string> &_chips) const {_chips = c_spi_dschips;};
    inline const void getThermocouples(std::map<std::string, int> &_tcs) const {_tcs = c_thermocouples;};
    inline const uint32_t getTCival() const { return c_thermoival;};
    inline const uint16_t getPRPort() const { return c_zmq_pr_port; };
  };
}

#endif
