/*
  Generic type declarations
 */

#include <cstdint>

namespace aegir {
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
    ThermoCouple(const char *_name);
    constexpr ThermoCouple(Value _v): c_value(_v) {};

    constexpr operator Value() const { return c_value; };
    explicit operator bool() const = delete;

  private:
    Value c_value;

  public:
    const char* toStr() const;
  };

}
