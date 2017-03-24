
#include "Message.hh"

#include <string.h>

#include <algorithm>

namespace aegir {

  /*
   * MessageFactoryReg
   */
  MessageFactoryReg::MessageFactoryReg(MessageType _type, ffunc_t _ffunc) {
    MessageFactory::getInstance().registerCtor(_type, _ffunc);
  }

  /*
   * MessageFactory
   */
  MessageFactory::MessageFactory() {
  }

  MessageFactory::~MessageFactory() {
  }

  MessageFactory &MessageFactory::getInstance() {
    static MessageFactory instance;
    return instance;
  }

  void MessageFactory::registerCtor(MessageType _type, ffunc_t _ctor) {
    c_ctors[(int)_type] = _ctor;
  }

  std::shared_ptr<Message> MessageFactory::create(const uint8_t *_msg) {
    return c_ctors[(int)_msg[0]](_msg);
  }

  /*
   * Message
   */
  Message::~Message() = default;


  std::string Message::hexdebug() const {
    auto msg = this->serialize();
    std::string hex;

    for ( auto &it: msg ) {
      char buff[8];
      int len;
      len = snprintf(buff, 4, " %02x", it);
      hex += std::string(buff, len);
    }

    return hex;
  }

  /*
   * PinStateMessage
   * Format is:
   * MessageType: 1 byte
   * Name: 1) stringsize: 1 byte + 2) stringsize-bytes
   * State: 1 byte
   */
  PinStateMessage::PinStateMessage(const uint8_t *_msg) {
  }

  PinStateMessage::PinStateMessage(const std::string &_name, int _state): c_name(_name), c_state(_state) {
  }

  PinStateMessage::~PinStateMessage() {
  }

  msgstring PinStateMessage::serialize() const {
    uint32_t len(3);
    uint32_t strsize(std::min((uint32_t)c_name.length(), (uint32_t)255));
    len += strsize;

    msgstring buffer(len, 0);
    uint8_t *data = (uint8_t*)buffer.data();
    data[0] = (uint8_t)type();
    data[1] = (uint8_t)strsize;
    data[len-1] = (int8_t)c_state;
    memcpy((void*)(data+2), (void*)c_name.c_str(), strsize);

    return buffer;
  }

  MessageType PinStateMessage::type() const {
    return MessageType::PINSTATE;
  }

  std::shared_ptr<Message> PinStateMessage::create(const uint8_t *_msg) {
    return std::make_shared<PinStateMessage>(_msg);
  }
}
