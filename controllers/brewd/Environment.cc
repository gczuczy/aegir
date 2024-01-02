
#include "Environment.hh"

namespace aegir {

  Environment::Environment() {
  }

  Environment::~Environment() {
  }

  std::shared_ptr<Environment> Environment::getInstance() {
    static std::shared_ptr<Environment> instance{new Environment()};
    return instance;
  }

  void Environment::setThermoReadings(ThermoReadings& _r) {
    for (uint8_t i=0; i<ThermoCouple::_SIZE; ++i) {
      switch (i) {
      case ThermoCouple::MT:
	c_temp_mt = _r[i];
	break;
      case ThermoCouple::RIMS:
	c_temp_rims = _r[i];
	break;
      case ThermoCouple::BK:
	c_temp_bk = _r[i];
	break;
      case ThermoCouple::HLT:
	c_temp_hlt = _r[i];
	break;
      }
    }
  }
}
