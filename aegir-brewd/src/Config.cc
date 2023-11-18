
#include "Config.hh"

#include <stdio.h>

#include <iostream>
#include <fstream>

#include <yaml-cpp/yaml.h>

#include "Exception.hh"
#include "logging.hh"

namespace aegir {

  // PIN default configuration
  pinconfig_t g_pinconfig = pinconfig_t{
    {"mtlevel", PinConfig(PinMode::IN, PinPull::DOWN)},
    {"cs0", PinConfig(PinMode::OUT, PinPull::NONE)},
    {"cs1", PinConfig(PinMode::OUT, PinPull::NONE)},
    {"cs2", PinConfig(PinMode::OUT, PinPull::NONE)},
    {"cs3", PinConfig(PinMode::OUT, PinPull::NONE)},
    {"mtpump", PinConfig(PinMode::OUT, PinPull::NONE)},
    {"bkpump", PinConfig(PinMode::OUT, PinPull::NONE)},
    {"mtheat", PinConfig(PinMode::OUT, PinPull::NONE)},
    {"buzzer", PinConfig(PinMode::OUT, PinPull::NONE)}
  };

  static std::set<std::string> g_tcnames{"HERMS", "MashTun", "HLT", "BK"};

  // SPI ChipSelector string translations
  static std::map<ChipSelectors, std::string> g_spi_cs_to_string{
    {ChipSelectors::DirectSelect, "DirectSelect"}};
  static std::map<std::string, ChipSelectors> g_spi_string_to_cs{
    {"DirectSelect",ChipSelectors::DirectSelect}};
  // ThermoCouple type lookups
  std::map<std::string, MAX31856::TCType> g_string_to_tctype{
    {"B", MAX31856::TCType::B},
    {"E", MAX31856::TCType::E},
    {"J", MAX31856::TCType::J},
    {"K", MAX31856::TCType::K},
    {"N", MAX31856::TCType::N},
    {"R", MAX31856::TCType::R},
    {"S", MAX31856::TCType::S},
    {"T", MAX31856::TCType::T}
  };
  std::map<MAX31856::TCType, std::string> g_tctype_to_string{
    {MAX31856::TCType::B, "B"},
    {MAX31856::TCType::E, "E"},
    {MAX31856::TCType::J, "J"},
    {MAX31856::TCType::K, "K"},
    {MAX31856::TCType::N, "N"},
    {MAX31856::TCType::R, "R"},
    {MAX31856::TCType::S, "S"},
    {MAX31856::TCType::T, "T"}
  };
  // Noisefilter lookups
  std::map<std::string, NoiseFilters> g_string_to_noisefilter{
    {"50Hz", NoiseFilters::HZ50},
    {"60Hz", NoiseFilters::HZ60}
  };
  std::map<NoiseFilters, std::string> g_noisefilter_to_string{
    {NoiseFilters::HZ50, "50Hz"},
      {NoiseFilters::HZ60, "60Hz"}
  };

  Config::Config() {
    setDefaults();
  }

  Config::~Config() {
    save();
  }

  std::shared_ptr<Config> Config::getInstance() {
    static std::shared_ptr<Config> instance{new Config()};
    return instance;
  }

  void Config::setDefaults() {
    c_device = "/dev/gpioc0";

    // PIN layout
    c_pinlayout.clear();
    c_pinlayout["mtlevel"] = 16;
    c_pinlayout["mtpump"] = 23;
    c_pinlayout["bkpump"] = 24;
    c_pinlayout["mtheat"] = 25;
    c_pinlayout["buzzer"] = 26;

    // SPI config
    c_spidev = "/dev/spigen0.0";
    c_spi_chipselector = ChipSelectors::DirectSelect;
    c_pinlayout["cs0"] = 7;
    c_pinlayout["cs1"] = 8;
    c_pinlayout["cs2"] = 5;
    c_pinlayout["cs3"] = 6;
    // SPI / MAX31856
    c_spi_max31856_tctype = MAX31856::TCType::T;
    c_spi_max31856_noisefilter = NoiseFilters::HZ50;
    // SPI / chips
    c_spi_dschips = {{0, "cs0"}, {1, "cs1"}, {2, "cs2"}, {3, "cs3"}};

    // thermocouples
    c_thermocouples.tcs[ThermoCouple::MT] = 1;
    c_thermocouples.tcs[ThermoCouple::HERMS] = 0;
    c_thermocouples.tcs[ThermoCouple::BK] = 3;
    c_thermocouples.tcs[ThermoCouple::HLT] = 2;

    // thermocouple reading interval
    c_thermoival = 1;

    // the PR ZMQ socket
    c_zmq_pr_port = 42069;

    // the heating element's power
    c_hepower = 9000;

    // pin polling interval
    c_pinival = 100;

    // temperature accuracy
    c_tempaccuracy = 0.3;

    // max heating overhead
    c_heatoverhead = 2.0;

    // heating element cycle time
    c_hecycletime = 3.0;

    // cooling temperature
    c_cooltemp = 24.0f;

    // HE startup delay
    c_hedelay = 10;

    // loglevel
    c_loglevel = blt::severity_level::info;
  }

  void Config::load(const std::string& _file) {
    c_cfgfile = _file;
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
      // GPIO
      if ( config["gpio"] && config["gpio"].IsMap() ) {
	YAML::Node gpio = config["gpio"];
	// load the device
	if ( gpio["device"] ) c_device = gpio["device"].as<std::string>();

	// the PIN configuration
	if ( gpio["pins"] && gpio["pins"].IsMap() ) {
	  YAML::Node pins = gpio["pins"];
	  std::map<std::string, int> pinlayout(c_pinlayout);
	  for (const auto &it: g_pinconfig)
	    if ( pins[it.first] ) pinlayout[it.first] = pins[it.first].as<int>();

	  checkPinConfig(pinlayout);
	  c_pinlayout = pinlayout;
	} // PIN config
      } // GPIO section

      // SPI
      if ( config["spi"] && config["spi"].IsMap() ) {
	YAML::Node spi = config["spi"];

	// load the device
	if ( spi["device"] ) c_spidev = spi["device"].as<std::string>();
	// load the selector
	if ( spi["selector"] ) {
	  std::string csname(spi["selector"].as<std::string>());
	  auto it = g_spi_string_to_cs.find(csname);
	  if ( it == g_spi_string_to_cs.end() ) {
	    throw Exception("Unknown SPI chip selector: %s\n", csname.c_str());
	  }
	  c_spi_chipselector = it->second;
	}
	// MAX31856 config
	if ( spi["MAX31856"] && spi["MAX31856"].IsMap() ) {
	  auto max31856 = spi["MAX31856"];
	  // TC type
	  if ( max31856["tctype"] ) {
	    std::string tctype = max31856["tctype"].as<std::string>();
	    auto it = g_string_to_tctype.find(tctype);
	    if ( it == g_string_to_tctype.end() ) {
	      throw Exception("Unknown TC type: %s\n", tctype.c_str());
	    }
	    c_spi_max31856_tctype = it->second;
	  }// TC type
	  // Noise Filter, AKA 50/60Hz
	  if ( max31856["noisefilter"] ) {
	    std::string nf = max31856["noisefilter"].as<std::string>();
	    auto it = g_string_to_noisefilter.find(nf);
	    if ( it == g_string_to_noisefilter.end() ) {
	      throw Exception("Unknown Noise Filter mode: %s\n", nf.c_str());
	    }
	    c_spi_max31856_noisefilter = it->second;
	  }// Noise Filter
	} // MAX31856 config
	// DirectSelect Chip Selector
	// the PIN configuration
	if ( spi["DirectSelect"] && spi["DirectSelect"].IsMap() ) {
	  YAML::Node ds = spi["DirectSelect"];

	  // we have 3 hardcoded sensors at this level, for now
	  for (int i=0; i<3; ++i) {
	    if ( !ds[i] ) continue;
	    std::string pin = ds[i].as<std::string>();
	    auto it = g_pinconfig.find(pin);
	    if ( it == g_pinconfig.end() )
	      throw Exception("Unknown pin: %s", pin.c_str());
	    c_spi_dschips[i] = pin;
	  }

	  // Now we have to check for dups
	  std::set<std::string> tmp;
	  for ( auto &it: c_spi_dschips ) {
	    auto it2 = tmp.find(it.second);
	    if ( it2 != tmp.end() )
	      throw Exception("Duplicate pin for DirectSelect: %s", it.second.c_str());
	    tmp.insert(it.second);
	  }
	} // PIN config

	// Thermocouples
	if ( spi["thermocouples"] && spi["thermocouples"].IsMap() ) {
	  YAML::Node tc = spi["thermocouples"];

	  // We are going through only the valid ones, the rest are ignored
	  // also, not emptying the list, because that's how unset values default
	  for (auto &it: g_tcnames) {
	    if ( tc[it] ) {
	      int id = tc[it].as<int>();
	      if ( id < 0 || id > 3 )
		throw Exception("Thermocouple id out of range: %i", id);
	      c_thermocouples.tcs[ThermoCouple(it)] = id;
	    }
	  }
	  // now verify whether any of them is on the same id
	  std::set<int> tmp;
	  for (uint8_t i=0; i < ThermoCouple::_SIZE; ++i) {
	    if ( tmp.find(c_thermocouples.tcs[i]) != tmp.end() )
	      throw Exception("Duplicate thermocouple id: %i", i);
	  }
	} // thermocouples
	if ( spi["thermointerval"] && spi["thermointerval"].IsScalar() ) {
	  YAML::Node ti = spi["thermointerval"];
	  c_thermoival = ti.as<uint32_t>();
	if ( c_thermoival > 60 )
	  throw Exception("Thermocouple reading interval is too high: %lu", c_thermoival);
	}
      } // SPI section

      // ZMQ PR socket
      if ( config["prport"] && config["prport"].IsScalar() ) {
	YAML::Node prsock = config["prport"];
	c_zmq_pr_port = prsock.as<uint16_t>();
      }

      // Heating Element's Power
      if ( config["elementpower"] && config["elementpower"].IsScalar() ) {
	YAML::Node hep = config["elementpower"];
	c_hepower = hep.as<uint32_t>();
	if ( c_hepower < 1000 || c_hepower > 10000 )
	  throw Exception("Heating element's power is out of range");
      }

      // pin polling interval
      if ( config["pinpollinterval"] && config["pinpollinterval"].IsScalar() ) {
	YAML::Node ppi = config["pinpollinterval"];
	c_pinival = ppi.as<uint32_t>();
	if ( c_pinival < 20 || c_pinival > 1000 )
	  throw Exception("Pin polling interval is out of range");
      }

      // temperature accuracy
      if ( config["tempaccuracy"] && config["tempaccuracy"].IsScalar() ) {
	YAML::Node ppi = config["tempaccuracy"];
	c_tempaccuracy = ppi.as<float>();
	if ( c_tempaccuracy < 0.1 || c_tempaccuracy > 3.0 )
	  throw Exception("Temperature accuracy is out of range");
      }

      // max heating overhead
      if ( config["heatoverhead"] && config["heatoverhead"].IsScalar() ) {
	YAML::Node ho = config["heatoverhead"];
	c_heatoverhead = ho.as<float>();
	if ( c_heatoverhead< 0.1 || c_heatoverhead > 5.0 )
	  throw Exception("Heating overhead is out of range");
      }

      // heating element cycle time
      if ( config["hecycletime"] && config["hecycletime"].IsScalar() ) {
	YAML::Node hect = config["hecycletime"];
	c_hecycletime = hect.as<float>();
	if ( c_hecycletime < 1.0 || c_hecycletime > 5.0 )
	  throw Exception("Heating element cycle time is out of range");
      }

      // cooling temperature
      if ( config["cooltemp"] && config["cooltemp"].IsScalar() ) {
	YAML::Node ct = config["cooltemp"];
	c_cooltemp = ct.as<float>();
	if ( c_cooltemp < 10.0f || c_cooltemp > 30.0 )
	  throw Exception("Cooling temperature must be between 10C and 30C");
      }

      // HE startup delay
      if ( config["hedelay"] && config["hedelay"].IsScalar() ) {
	YAML::Node ct = config["hedelay"];
	c_hedelay = ct.as<uint32_t>();
	if ( c_hedelay > 30.0 )
	  throw Exception("HE startup delay must be less than 30");
      }

      // LogLevel
      if ( config["loglevel"] && config["loglevel"].IsScalar() ) {
	std::string level = config["loglevel"].as<std::string>();
	setLogLevel(level);
      }

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

  Config &Config::save(const std::string& _file) {
    c_cfgfile = _file;
    return save();
  }

  Config &Config::save() {
    // don't save if we don't have a file but the defaults
    if ( c_cfgfile.empty() ) return *this;
    YAML::Emitter yout;

    yout << YAML::BeginMap;
    // GPIO config
    yout << YAML::Key << "gpio";
    yout << YAML::Value << YAML::BeginMap;
    yout << YAML::Key << "device" << YAML::Value << c_device;
    yout << YAML::Key << "pins" << YAML::Value << c_pinlayout;
    yout << YAML::EndMap;

    // SPI config
    yout << YAML::Key << "spi";
    yout << YAML::Value << YAML::BeginMap;
    yout << YAML::Key << "device" << YAML::Value << c_spidev;
    yout << YAML::Key << "selector" << YAML::Value << g_spi_cs_to_string[c_spi_chipselector];

    // SPI / MAX31856 config
    yout << YAML::Key << "MAX31856";
    yout << YAML::Value <<
      std::map<std::string, std::string>{
      {"tctype", g_tctype_to_string[c_spi_max31856_tctype]},
	{"noisefilter", g_noisefilter_to_string[c_spi_max31856_noisefilter]}};
    // end SPI / MAX31856

    // SPI / DirectSelect
    yout << YAML::Key << "DirectSelect";
    yout << YAML::Value << c_spi_dschips;

    // Thermocouple layout
    yout << YAML::Key << "thermocouples";
    {
      std::map<std::string, int> tcmap;
      for (uint8_t i=0; i < ThermoCouple::_SIZE; ++i)
	tcmap[ThermoCouple(i).toStr()] = c_thermocouples.tcs[i];
      yout << YAML::Value << tcmap;
    }

    // thermocouple reading interval
    yout << YAML::Key << "thermointerval";
    yout << YAML::Value << c_thermoival;

    yout << YAML::EndMap; // end of SPI

    // ZMQ PR port
    yout << YAML::Key << "prport" << YAML::Value << c_zmq_pr_port;

    // Heating element's power in watts
    yout << YAML::Key << "elementpower" << YAML::Value << c_hepower;

    // pin polling interval
    yout << YAML::Key << "pinpollinterval" << YAML::Value << c_pinival;

    // temperature accuracy
    yout << YAML::Key << "tempaccuracy" << YAML::Value << c_tempaccuracy;

    // max heating overhead
    yout << YAML::Key << "heatoverhead" << YAML::Value << c_heatoverhead;

    // max heating overhead
    yout << YAML::Key << "hecycletime" << YAML::Value << c_hecycletime;

    // cooling temperature
    yout << YAML::Key << "cooltemp" << YAML::Value << c_cooltemp;

    // cooling temperature
    yout << YAML::Key << "hedelay" << YAML::Value << c_hedelay;

    // loglevel
    yout << YAML::Key << "loglevel"
	 << YAML::Value << logging::str(c_loglevel);

    // End the config
    yout << YAML::EndMap;

    std::ofstream outfile;

    outfile.open(c_cfgfile, std::ios::out | std::ios::trunc);
    outfile << yout.c_str() << std::endl;
    outfile.close();

    return *this;
  }

  const std::string &Config::getGPIODevice() const {
    return c_device;
  }

  Config &Config::getPinConfig(pinlayout_t &_layout) {
    _layout = c_pinlayout;
    return *this;
  }

  Config &Config::setHEPower(uint32_t _v) {
    c_hepower = _v;
    return *this;
  }

  Config &Config::setTempAccuracy(float _v) {
    c_tempaccuracy = _v;
    return *this;
  }

  Config &Config::setHeatOverhead(float _v) {
    c_heatoverhead = _v;
    return *this;
  }

  Config &Config::setCoolTemp(float _v) {
    c_cooltemp = _v;
    return *this;
  }

  Config &Config::setHEDelay(uint32_t _v) {
    c_hedelay = _v;
    return *this;
  }

  Config &Config::setLogLevel(const std::string& _level) {
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

  Config &Config::setLogLevel(blt::severity_level _level) {
    c_loglevel = _level;
    return *this;
  }
}
