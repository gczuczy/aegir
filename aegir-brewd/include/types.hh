/*
  Generic type declarations
 */

#include <cstdint>

namespace aegir {
  enum class ThermoSensors: uint8_t {
    MT=0, // mashtun
    HERMS, // herms tube
    BK, // boil kettle
    HLT, // hot liquor tank
    _SIZE // unused, indicates the size
  };
}
