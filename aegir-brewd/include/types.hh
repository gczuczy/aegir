/*
  Generic type declarations
 */

#ifndef AEGIR_TYPES_H
#define AEGIR_TYPES_H

#include <cstdint>
#include <string>
#include <ostream>
#include <iomanip>

namespace aegir {

  /*
    ThermoCouple
   */
  class ThermoCouple {
  public:
    enum Value: uint8_t {
      MT=0, // mashtun
      HERMS, // herms tube
      RIMS=HERMS,
      BK, // boil kettle
      HLT, // hot liquor tank
      _SIZE // unused, indicates the size
    };

    ThermoCouple() = default;
    ThermoCouple(const uint8_t _i);
    ThermoCouple(const char *_name);
    ThermoCouple(const std::string &_name);
    constexpr ThermoCouple(Value _v): c_value(_v) {};

    constexpr operator Value() const { return c_value; };
    explicit operator bool() const = delete;

  private:
    Value c_value;

  public:
    const char* toStr() const;
  };

  struct ThermoReadings {
    float data[ThermoCouple::_SIZE];
    inline float& operator[](std::size_t _idx) {return data[_idx];};
    inline const float& operator[](std::size_t _idx) const {return data[_idx];};
    friend std::ostream& operator<<(std::ostream& os, const ThermoReadings& r) {
      os << "ThermoReadings(";
      for (uint8_t i=0; i<ThermoCouple::_SIZE;++i) {
	if ( i == 0 ) {
	  os << ThermoCouple(i).toStr()<< ":"
	     << std::fixed
	     << std::setw(5) << std::setprecision(2)
	     << r.data[i];
	} else {
	  os << ", " << ThermoCouple(i).toStr()<< ":"
	     << std::fixed
	     << std::setw(5) << std::setprecision(2)
	     << r.data[i];
	}
      }
      os << ")";
      return os;
    }
  };

  /*
    PINState
   */
  enum class PINState: uint8_t {
    Off=0,
    On=1,
    Pulsate=2,
    Unknown=255
  };

}

#endif
