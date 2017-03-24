/*
 * Message representation for MQ communication
 */

#ifndef AEGIR_MESSAGE_H
#define AEGIR_MESSAGE_H

#include <cstdint>
#include <string>
#include <memory>
#include <functional>

namespace aegir {

  typedef std::basic_string<uint8_t> msgstring;

  enum class MessageType:uint8_t {
    UNKNOWN=0,
    PINSTATE=1,
  };

  class Message {
  public:
    std::string hexdebug() const;
    virtual msgstring serialize() const = 0;
    virtual MessageType type() const = 0;
    virtual ~Message() = 0;
  };

  typedef std::function<std::shared_ptr<Message>(const uint8_t*)> ffunc_t;

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
    std::shared_ptr<Message> create(const uint8_t *_msg);
  };

  class PinStateMessage: public Message {
  public:
    PinStateMessage() = delete;
    PinStateMessage(const uint8_t *_msg);
    PinStateMessage(const std::string &_name, int _state);
    virtual msgstring serialize() const override;
    virtual MessageType type() const override;
    virtual ~PinStateMessage();

    static std::shared_ptr<Message> create(const uint8_t*);

  public:
    std::string c_name;
    int c_state;
  };

}

#endif
