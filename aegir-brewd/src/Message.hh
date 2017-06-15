/*
 * Message representation for MQ communication
 */

#ifndef AEGIR_MESSAGE_H
#define AEGIR_MESSAGE_H

#include <cstdint>
#include <string>
#include <memory>
#include <functional>

#include "GPIO.hh"

namespace aegir {

  typedef std::basic_string<uint8_t> msgstring;

  enum class MessageType:uint8_t {
    UNKNOWN=0,
      PINSTATE=1,
      THERMOREADING=2,
      JSON=3
  };

  const std::string hexdump(const msgstring &_msg);
  const std::string hexdump(const uint8_t *_msg, int _size);

  class Message {
  public:
    std::string hexdebug() const;
    virtual msgstring serialize() const = 0;
    virtual MessageType type() const = 0;
    virtual ~Message() = 0;
  };

  typedef std::function<std::shared_ptr<Message>(const msgstring&)> ffunc_t;

  class MessageFactoryReg {
  public:
    MessageFactoryReg(MessageType _type, ffunc_t _ffunc);
    MessageFactoryReg() = delete;
    MessageFactoryReg(MessageFactoryReg&&) = delete;
    MessageFactoryReg(const MessageFactoryReg &) = delete;
    MessageFactoryReg &operator=(MessageFactoryReg &&) = delete;
    MessageFactoryReg &operator=(const MessageFactoryReg &) = delete;
  };

  class MessageFactory {
    friend MessageFactoryReg;
  private:
    MessageFactory();
    MessageFactory(MessageFactory&&) = delete;
    MessageFactory(const MessageFactory &) = delete;
    MessageFactory &operator=(MessageFactory&&) = delete;
    MessageFactory &operator=(const MessageFactory &) = delete;

  private:
    ffunc_t c_ctors[256];

  protected:
    void registerCtor(MessageType _type, ffunc_t _ctor);

  public:
    ~MessageFactory();
    static MessageFactory &getInstance();
    std::shared_ptr<Message> create(const msgstring &_msg);
  };

  // This communicates a GPIO pin's state
  class PinStateMessage: public Message {
  public:
    PinStateMessage() = delete;
    PinStateMessage(const msgstring &_msg);
    PinStateMessage(const std::string &_name, PINState _state, float _cycletime=3.0f, float _onratio=0.2f);
    virtual msgstring serialize() const override;
    virtual MessageType type() const override;
    inline const std::string &getName() const {return c_name;};
    inline PINState getState() const {return c_state;};
    inline bool isOn() const {return c_state==PINState::On; };
    inline bool isOff() const {return c_state==PINState::Off; };
    virtual ~PinStateMessage();

    static std::shared_ptr<Message> create(const msgstring &_msg);

  public:
    std::string c_name;
    PINState c_state;
    float c_cycletime;
    float c_onratio;
  };

  // Thermocouple reading results
  class ThermoReadingMessage: public Message {
  public:
    ThermoReadingMessage() = delete;
    ThermoReadingMessage(const msgstring &_msg);
    ThermoReadingMessage(const std::string &_name, float _temp, uint32_t _timestamp);
    virtual msgstring serialize() const override;
    virtual MessageType type() const override;
    inline const std::string &getName() const {return c_name;};
    inline float getTemp() const {return c_temp;};
    inline uint32_t getTimestamp() const {return c_timestamp;};
    virtual ~ThermoReadingMessage();

    static std::shared_ptr<Message> create(const msgstring &_msg);

  public:
    std::string c_name;
    float c_temp;
    uint32_t c_timestamp;
  };
}

#endif
