/*
   misc function for types.hh
 */

#include <stdio.h>
#include <cstring>

#include "types.hh"
#include "Exception.hh"

namespace aegir {
  struct alignas(sizeof(long)) sensorstr {
    ThermoCouple sensor;
    const char* name;
    bool primary;
  };
  static sensorstr g_sensornames[] = {
    {ThermoCouple::MT, "MT", false},
    {ThermoCouple::MT, "MashTun", true},
    {ThermoCouple::HERMS, "HERMS", true},
    {ThermoCouple::RIMS, "RIMS", true},
    {ThermoCouple::BK, "BoilKettle", true},
    {ThermoCouple::BK, "BK", false},
    {ThermoCouple::HLT, "HLT", false},
    {ThermoCouple::HLT, "HotLiquorTank", true},
  };

  ThermoCouple::ThermoCouple(const uint8_t _i) {
    for (int i=0; i < sizeof(g_sensornames)/sizeof(sensorstr); ++i ) {
      if ( g_sensornames[i].sensor == _i ) {
	c_value = g_sensornames[i].sensor;
	return;
      }
    }
    throw Exception("Unknown sensor id %i", _i);
  }

  ThermoCouple::ThermoCouple(const char *_name) {
    for (int i=0; i < sizeof(g_sensornames)/sizeof(sensorstr); ++i ) {
      if ( std::strcmp(g_sensornames[i].name, _name) == 0 ) {
	c_value = g_sensornames[i].sensor;
	return;
      }
    }
    throw Exception("Unknown sensor '%s'", _name);
  }

  ThermoCouple::ThermoCouple(const std::string &_name) {
    for (int i=0; i < sizeof(g_sensornames)/sizeof(sensorstr); ++i ) {
      if ( std::strcmp(g_sensornames[i].name, _name.c_str()) == 0 ) {
	c_value = g_sensornames[i].sensor;
	return;
      }
    }
    throw Exception("Unknown sensor '%s'", _name.c_str());
  }

  const char* ThermoCouple::toStr() const {
    for (int i=0; i < sizeof(g_sensornames)/sizeof(sensorstr); ++i ) {
      if ( g_sensornames[i].sensor == c_value && g_sensornames[i].primary )
	return g_sensornames[i].name;
    }
    throw Exception("ThermoCouple value has reserved use");
  }
}
