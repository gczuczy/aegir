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
  };
  static sensorstr g_sensornames[] = {
    {ThermoCouple::MT, "MT"},
    {ThermoCouple::HERMS, "HERMS"},
    {ThermoCouple::RIMS, "RIMS"},
    {ThermoCouple::BK, "BK"},
    {ThermoCouple::HLT, "HLT"},
  };

  ThermoCouple::ThermoCouple(const char *_name) {
    for (int i=0; i < sizeof(g_sensornames)/sizeof(sensorstr); ++i ) {
      if ( std::strcmp(g_sensornames[i].name, _name) == 0 ) {
	c_value = g_sensornames[i].sensor;
	return;
      }
    }
    throw Exception("Unknown sensor '%s'", _name);
  }

  const char* ThermoCouple::toStr() const {
    for (int i=0; i < sizeof(g_sensornames)/sizeof(sensorstr); ++i ) {
      if ( g_sensornames[i].sensor == c_value )
	return g_sensornames[i].name;
    }
    throw Exception("ThermoCouple value has reserved use");
  }
}
