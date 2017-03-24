/*
  Basic C++ wrapper for libgpio
 */

#ifndef GPIO_HH
#define GPIO_HH

#include <sys/types.h>
#include <libgpio.h>

#include <string>
#include <map>
#include <vector>

#include "Exception.hh"

namespace aegir {

  class GPIO {
    GPIO() = delete;
    GPIO(GPIO &&) = delete;
    GPIO(const GPIO &) = delete;
    GPIO &operator=(GPIO &&) = delete;
    GPIO &operator=(const GPIO &) = delete;

  private:
    GPIO(int _unit);
    GPIO(const std::string _device);
  public:
    static GPIO *getInstance();
    ~GPIO();

    // PIN class for pin access
    class PIN {
      friend GPIO;
      PIN() = delete;
    protected:
      PIN(GPIO &_gpio, int _pin);
    private:
      GPIO &c_gpio;
      int c_pin;
    public:
      ~PIN();
      PIN &input();
      PIN &output();
      PIN &high();
      PIN &low();
      PIN &toggle();
      PIN &pullup();
      PIN &pulldown();
      PIN &opendrain();
      PIN &tristate();
      int get();
    private:
      // In this app we don't need setname to be public,
      // because names are set up from Config
      PIN &setname(const std::string &);
    };
    friend GPIO::PIN;

    // public functions
    PIN &operator[](const int _pin);
    PIN &operator[](const std::string &_name);
    std::vector<std::string> getPinNames() const;

  protected:
    gpio_handle_t c_handle;
    void setname(int _pin, const std::string &_name);

  private:
    static GPIO *c_instance;
    std::map<int, PIN> c_pins;
    std::map<std::string, int> c_names;

    void fetchpins();
    void rewire();
  };

}

#endif
