
#include "MAX31856.hh"

#include <stdio.h>

#include <chrono>
#include <thread>
#include <list>

namespace aegir {

  MAX31856::MAX31856(SPI &_spi, int _chipid): c_spi(_spi), c_chipid(_chipid), c_convmode(false) {
    setConversionMode(false);

    // clear the mask register
    SPI::Data cmd{0x80|(uint8_t)Register::MASK}, data{0x00};
    xfer(cmd, data);
    // clear the ocfault registers
    clearOCFault();
  }

  MAX31856::~MAX31856() {
    setConversionMode(false);
  }

  MAX31856 &MAX31856::setConversionMode(bool _cmode) {
    SPI::Data cmd{(uint8_t)Register::CR0}, data(1);

    c_convmode = _cmode;

    xfer(cmd, data);
#ifdef MAX31856_DEBUG
    printf("setConvMode(%i): Pre-state: %s\n", c_chipid, data.hexdump().c_str());
#endif
    uint8_t modebit = _cmode ? 0b10000000 : 0;
    data[0] &= 0x7f;
    data[0] |= modebit;
#ifdef MAX31856_DEBUG
    printf("setConvMode(%i): Setting to %s MB:%02x\n", c_chipid, data.hexdump().c_str(), modebit);
#endif
    cmd[0] = 0x80|(uint8_t)Register::CR0;

    xfer(cmd, data);

    return *this;
  }

  MAX31856 &MAX31856::set50Hz(bool _50hz) {
    SPI::Data cmd{(uint8_t)Register::CR0}, data(1);

    xfer(cmd, data);
#ifdef MAX31856_DEBUG
    printf("set50Hz(%i): Pre-state: %s\n", c_chipid, data.hexdump().c_str());
#endif
    uint8_t modebit = _50hz ? 0b00000001 : 0;
    data[0] &= 0xfd;
    data[0] |= modebit;
#ifdef MAX31856_DEBUG
    printf("set50Hz(%i): Setting to %s MB:%02x\n", c_chipid, data.hexdump().c_str(), modebit);
#endif
    cmd[0] = 0x80|(uint8_t)Register::CR0;

    xfer(cmd, data);

    return *this;
  }

  void MAX31856::clearOCFault() {
    SPI::Data cmd{(uint8_t)Register::CR0}, data(1);

    xfer(cmd, data);
#ifdef MAX31856_DEBUG
    printf("clearOCFault(%i): Pre-state: %s\n", c_chipid, data.bindump().c_str());
#endif
    data[0] &= 0b11001111;
#ifdef MAX31856_DEBUG
    printf("clearOCFault(%i): Setting to %s\n", c_chipid, data.bindump().c_str());
#endif
    cmd[0] = 0x80|(uint8_t)Register::CR0;

    xfer(cmd, data);
  }

  MAX31856 &MAX31856::setAvgMode(MAX31856::AvgMode _mode) {
    if ( c_convmode ) {
      printf("Don't set AvgMode while conversion mode is on");
      throw std::exception();
    }

    SPI::Data cmd{(uint8_t)Register::CR1}, data(1);

    uint8_t mask = 0b10001111;
    uint8_t modebit = (uint8_t)_mode;

    xfer(cmd, data);
#ifdef MAX31856_DEBUG
    printf("setAvgMode(%i): Pre-state: %s/%s\n", c_chipid, data.hexdump().c_str(),
	   data.bindump().c_str());
#endif
    data[0] &= mask;
    data[0] |= modebit<<4;
#ifdef MAX31856_DEBUG
    printf("setAvgMode(%i): Setting to %s/%s\n", c_chipid, data.hexdump().c_str(),
	   data.bindump().c_str());
#endif
    cmd[0] = 0x80|(uint8_t)Register::CR1;

    xfer(cmd, data);

    return *this;
  }

  MAX31856 &MAX31856::setTCType(TCType _type) {
    SPI::Data cmd{(uint8_t)Register::CR1}, data(1);

    uint8_t mask = 0b11110000;
    uint8_t modebit = (uint8_t)_type;

    xfer(cmd, data);
#ifdef MAX31856_DEBUG
    printf("setTCType(%i): Pre-state: %s/%s\n", c_chipid, data.hexdump().c_str(),
	   data.bindump().c_str());
#endif
    data[0] &= mask;
    data[0] |= modebit;
#ifdef MAX31856_DEBUG
    printf("setTCType(%i): Setting to %s/%s, c_chipidn", c_chipid, data.hexdump().c_str(),
	   data.bindump().c_str());
#endif
    cmd[0] = 0x80|(uint8_t)Register::CR1;

    xfer(cmd, data);

    return *this;
  }

  MAX31856 &MAX31856::setCJOffset(float _offset) {
    SPI::Data cmd{(uint8_t)Register::CJTO}, data(1);

    uint8_t mask = 0b11110000;
    uint8_t offset = ((_offset<0)?1<<8:0)|(uint8_t)(_offset*16);

    printf("setCJOffset(%i, %.4f): offset:%u\n", c_chipid, _offset, offset);

    data[0] = offset;
#ifdef MAX31856_DEBUG
    printf("setTCType(%i): Setting to %s/%s, c_chipidn", c_chipid, data.hexdump().c_str(),
	   data.bindump().c_str());
#endif
    cmd[0] = 0x80|(uint8_t)Register::CJTO;

    xfer(cmd, data);

    return *this;
  }

  MAX31856 &MAX31856::dumpState() {
    printf("Dumping MAX31856 id:%i\n", c_chipid);
    for (auto &it: std::list<Register>({
	  Register::CR0,
	    Register::CR1,
	    Register::MASK,
	    //    Register::,
	    Register::SR
	    })) {
      SPI::Data cmd{(uint8_t)it}, data(1);
      xfer(cmd, data);
      printf(" - %02x: %s / %s\n ", (uint8_t)it,
	     data.hexdump().c_str(), data.bindump().c_str());
    }

    return *this;
  }

  void MAX31856::xfer(SPI::Data &_cmd, SPI::Data &_data) {
    c_spi.transfer(c_chipid, _cmd, _data);
  }

  float MAX31856::readCJTemp() {
    float temp;
    uint16_t reading(0);
    SPI::Data cmd{(uint8_t)Register::CJTH}, data(1);

    for (int i=0; i<2; ++i) {
      cmd[0] = (uint8_t)Register::CJTH + i;
      xfer(cmd, data);
      reading <<= 8;
      reading += data[0];
    }

    temp = (1.0*reading)/256.0;

    return temp;
  }

  float MAX31856::readTCTemp() {
    float temp=0;
    uint32_t reading(0);

#ifdef MAX31856_DEBUG
    dumpState();
#endif

    // if autoconv mode is disabled, we have to do a 1shot
    if ( !c_convmode ) {
      setOneShot();
      std::chrono::microseconds ival(175000);
      std::this_thread::sleep_for(ival);
    }

    SPI::Data cmd{(uint8_t)Register::LTCBH}, data(1);

    for (int i=0; i<3; ++i) {
      cmd[0] = (uint8_t)Register::LTCBH + i;
      xfer(cmd, data);
      reading <<= 8;
      reading += data[0];
    }
#ifdef MAX31856_DEBUG
    printf("MAX31856::readTCTemp(%i) native: %02x\n", c_chipid, reading);
#endif

    reading >>= 5;
#ifdef MAX31856_DEBUG
    printf("MAX31856::readTCTemp(%i) shr5: %02x\n", c_chipid, reading);
#endif
    temp = (1.0*reading)/128.0;
#ifdef MAX31856_DEBUG
    printf("MAX31856::readTCTemp(%i) /128: %.2f\n", c_chipid, temp);
    dumpState();
#endif
    return temp;
  }

  float MAX31856::getCJOffset() {
    float offset;
    uint8_t reading(0);
    SPI::Data cmd{(uint8_t)Register::CJTO}, data(1);

    xfer(cmd, data);
    reading = data[0];

    //printf("getCJOffset(%i): CJTO:%u/%s\n", c_chipid, reading, data.bindump().c_str());

    offset = (1.0*(reading&0x7f))/16.0;
    if ( reading&0x80 ) offset *= -1;

    return offset;
  }

  void MAX31856::setOneShot() {
    SPI::Data cmd{(uint8_t)Register::CR0}, data(1);

    uint8_t mask = 0b10111111;
    uint8_t modebit = (uint8_t)0b01000000;

    xfer(cmd, data);
#ifdef MAX31856_DEBUG
    printf("setOneShot(%i): Pre-state: %s\n", c_chipid, data.bindump().c_str());
#endif
    data[0] |= modebit;
#ifdef MAX31856_DEBUG
    printf("setOneShot(%i): Setting to %s\n", c_chipid, data.bindump().c_str());
#endif
    cmd[0] = 0x80|(uint8_t)Register::CR0;

    xfer(cmd, data);
  }
}

