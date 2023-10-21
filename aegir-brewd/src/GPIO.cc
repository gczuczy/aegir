
#include "GPIO.hh"

#ifdef GPIO_DEBUG
#include <stdio.h>
#endif
#include <stdlib.h>
#include <unistd.h>

#include "Config.hh"

namespace aegir {
  /*
    GPIO::PIN per-pin class
  */
  GPIO::PIN::PIN(GPIO &_gpio, int _pin): c_gpio(_gpio), c_pin(_pin) {
  }

  GPIO::PIN::~PIN() {
  }

  GPIO::PIN &GPIO::PIN::input() {
#ifdef GPIO_DEBUG
    printf("GPIO::PIN::%s(%i)\n", __FUNCTION__, c_pin);
#endif
    gpio_pin_input(c_gpio.c_handle, c_pin);
    return *this;
  }

  GPIO::PIN &GPIO::PIN::output() {
#ifdef GPIO_DEBUG
    printf("GPIO::PIN::%s(%i)\n", __FUNCTION__, c_pin);
#endif
    gpio_pin_output(c_gpio.c_handle, c_pin);
    return *this;
  }

  GPIO::PIN &GPIO::PIN::high() {
#ifdef GPIO_DEBUG
    printf("GPIO::PIN::%s(%i)\n", __FUNCTION__, c_pin);
#endif
    gpio_pin_high(c_gpio.c_handle, c_pin);
    return *this;
  }

  GPIO::PIN &GPIO::PIN::low() {
#ifdef GPIO_DEBUG
    printf("GPIO::PIN::%s(%i)\n", __FUNCTION__, c_pin);
#endif
    gpio_pin_low(c_gpio.c_handle, c_pin);
    return *this;
  }

  GPIO::PIN &GPIO::PIN::toggle() {
#ifdef GPIO_DEBUG
    printf("GPIO::PIN::%s(%i)\n", __FUNCTION__, c_pin);
#endif
    gpio_pin_toggle(c_gpio.c_handle, c_pin);
    return *this;
  }

  GPIO::PIN &GPIO::PIN::pullup() {
#ifdef GPIO_DEBUG
    printf("GPIO::PIN::%s(%i)\n", __FUNCTION__, c_pin);
#endif
    gpio_pin_pullup(c_gpio.c_handle, c_pin);
    return *this;
  }

  GPIO::PIN &GPIO::PIN::pulldown() {
#ifdef GPIO_DEBUG
    printf("GPIO::PIN::%s(%i)\n", __FUNCTION__, c_pin);
#endif
    gpio_pin_pulldown(c_gpio.c_handle, c_pin);
    return *this;
  }

  GPIO::PIN &GPIO::PIN::opendrain() {
#ifdef GPIO_DEBUG
    printf("GPIO::PIN::%s(%i)\n", __FUNCTION__, c_pin);
#endif
    gpio_pin_opendrain(c_gpio.c_handle, c_pin);
    return *this;
  }

  GPIO::PIN &GPIO::PIN::tristate() {
#ifdef GPIO_DEBUG
    printf("GPIO::PIN::%s(%i)\n", __FUNCTION__, c_pin);
#endif
    gpio_pin_tristate(c_gpio.c_handle, c_pin);
    return *this;
  }

  GPIO::PIN &GPIO::PIN::setname(const std::string &_name) {
    gpio_pin_set_name(c_gpio.c_handle, c_pin, (char*)_name.c_str());
    c_gpio.setname(c_pin, _name);
    return *this;
  }

  PINState GPIO::PIN::get() {
    int value;

    value = gpio_pin_get(c_gpio.c_handle, c_pin);

    if ( value == 0 ) return PINState::Off;
    if ( value == 1 ) return PINState::On;
    throw Exception("Unknown value for pin %i: %i", c_pin, value);
  }

  /*
    GPIO: The main class
  */
  GPIO *GPIO::c_instance = 0;

  GPIO::GPIO(int _unit) {
    c_handle = gpio_open(_unit);

    if ( c_handle == GPIO_INVALID_HANDLE ) {
      throw Exception("GPIO: invalid handle");
    }

    fetchpins();
    rewire();
  }

  GPIO::GPIO(const std::string _device) {
    c_handle = gpio_open_device(_device.c_str());

    if ( c_handle == GPIO_INVALID_HANDLE ) {
      throw Exception("GPIO: gpio_open_device(%s): invalid handle", _device.c_str());
    }

    fetchpins();
    rewire();
  }

  GPIO *GPIO::getInstance() {
    if ( !c_instance ) {
      Config *cfg = Config::getInstance();
      c_instance = new GPIO(cfg->getGPIODevice());
    }
    return c_instance;
  }


  GPIO::~GPIO() {
    gpio_close(c_handle);
  }

  void GPIO::setname(int _pin, const std::string &_name) {
    c_names[_name] = _pin;
  }

  void GPIO::fetchpins() {
    gpio_config_t *pinlist(0);
    int pinret;

    c_pins.clear();
    pinret = gpio_pin_list(c_handle, &pinlist);
    for ( int i=0; i < pinret; ++i ) {
      c_pins.insert(std::make_pair(pinlist[i].g_pin, PIN(*this, pinlist[i].g_pin)));
    }

    if ( pinlist ) free(pinlist);
  }

  // here we already have the device from the ctor
  // so we're only dealing with the pins
  void GPIO::rewire() {
    Config *cfg = Config::getInstance();

    // re-do the name-number association first
    c_names.clear();
    cfg->getPinConfig(c_names);

    // now set them up
    for (auto &it: c_names) {
      auto cfg = g_pinconfig[it.first];
      auto pin = c_pins.find(it.second)->second;

      if ( cfg.mode == PinMode::OUT ) {
	pin.output();
	if ( cfg.defval ) pin.high();
	else pin.low();
      } else if ( cfg.mode == PinMode::IN ) {
	pin.input();
	if ( cfg.pull == PinPull::DOWN ) pin.pulldown();
	else if ( cfg.pull == PinPull::UP ) pin.pullup();
      }
    }
  }

  GPIO::PIN &GPIO::operator[](const std::string &_name) {
    auto it = c_names.find(_name);

    if ( it == c_names.end() ) throw Exception("PIN not found: %s", _name.c_str());

    return (*this)[it->second];
  }

  GPIO::PIN &GPIO::operator[](const int _pin) {
    auto it = c_pins.find(_pin);

    if ( it == c_pins.end() ) throw Exception("Unable to find pin %i", _pin);

    return it->second;
  }

  std::vector<std::string> GPIO::getPinNames() const {
    std::vector<std::string> names;

    for (auto &it: c_names) names.push_back(it.first);

    return names;
  }
}
