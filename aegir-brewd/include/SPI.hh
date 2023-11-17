/*
 * SPI abstraction layer
 */


#ifndef AEGIR_SPI_H
#define AEGIR_SPI_H

#include "GPIO.hh"

#include <cstdint>
#include <string>
#include <initializer_list>

#include "LogChannel.hh"

namespace aegir {

  class ChipSelector {
  public:
    virtual void high(int _id) = 0;
    virtual void low(int _id) = 0;
    virtual ~ChipSelector() = 0;
  };

  class CSGuard {
  public:
    CSGuard(ChipSelector &_cs, int _id);
    ~CSGuard();
  private:
    ChipSelector &c_cs;
    int c_id;
  };

  class SPI {
  public:
    class Data {
    public:
      Data(int _size);
      Data(std::initializer_list<uint8_t> _il);
      ~Data();
      inline uint8_t *data() const {return c_data;};
      inline int size() const {return c_size;};
      uint8_t &operator[](const int _idx);
      std::string hexdump() const;
      std::string bindump() const;
    private:
      int c_size;
      uint8_t *c_data;
    };
  public:
    SPI(ChipSelector &_cs, GPIO &_gpio, const std::string &_spidev);
    ~SPI();
    void transfer(int _chipid, Data &_cmd, Data &_data);

  private:
    ChipSelector &c_cs;
    GPIO &c_gpio;
    int c_spifd;
    LogChannel c_log;
  };

}

#endif
