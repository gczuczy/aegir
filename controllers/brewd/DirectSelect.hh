/*
 * SPI CS selector, directly uses the CSn pins
 */

#ifndef AEGIR_DIRECTSELECT_H
#define AEGIR_DIRECTSELECT_H

#include "SPI.hh"
#include <map>

namespace aegir {

  class DirectSelect: public ChipSelector {
  public:
    DirectSelect(GPIO &_gpio, const std::map<int, std::string> &_chips);
    virtual void high(int _id);
    virtual void low(int _id);
    virtual ~DirectSelect();

  private:
    GPIO &c_gpio;
    std::map<int, std::string> c_chips;
  };
}

#endif
