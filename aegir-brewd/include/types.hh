/*
  Generic type declarations
 */

#ifndef AEGIR_TYPES_H
#define AEGIR_TYPES_H

#include <cstdint>
#include <string>

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
