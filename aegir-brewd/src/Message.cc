
#include "Message.hh"

#include <string.h>

#include <algorithm>

#include "Exception.hh"

namespace aegir {

  const std::string hexdump(const msgstring &_msg) {
    return hexdump(_msg.data(), _msg.length());
  }

  const std::string hexdump(const uint8_t *_msg, int _size) {
    std::string hex;

    for (int i=0; i<_size; ++i ) {
      char buff[8];
      int len;
      len = snprintf(buff, 4, " %02x", _msg[i]);
      hex += std::string(buff, len);
    }

    return hex;
  }


  MessageFactoryReg PinStateReg(MessageType::PINSTATE, PinStateMessage::create);

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

  std::shared_ptr<Message> MessageFactory::create(const msgstring &_msg) {
    int idx = (int)_msg[0];
    if ( c_ctors[idx] == nullptr ) throw Exception("Unknown message type %i", (int)_msg[0]);
    return c_ctors[idx](_msg);
  }

  /*
   * Message
   */
  Message::~Message() = default;

  std::string Message::hexdebug() const {
    return hexdump(this->serialize());
  }

  /*
   * PinStateMessage
   * Format is:
   * MessageType: 1 byte
   * Name: 1) stringsize: 1 byte + 2) stringsize-bytes
   * State: 1 byte
   */
  PinStateMessage::PinStateMessage(const msgstring &_msg) {
#ifdef AEGIR_DEBUG
    printf("PinStateMessage(L:%lu '%s') called\n", _msg.length(), hexdump(_msg).c_str());
#endif
    // _msg[0] is the type, but we're already here
    int msglen = _msg.length();

    uint8_t namelen = _msg[1];
    c_name = std::string((char*)_msg.data()+2, namelen);

    c_state = _msg[msglen-1];
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
    uint8_t *data = (uint8_t*)buffer.c_str();
    data[0] = (uint8_t)type();
    data[1] = (uint8_t)strsize;
    data[len-1] = (int8_t)c_state;
    memcpy((void*)(data+2), (void*)c_name.data(), strsize);

    return buffer;
  }

  MessageType PinStateMessage::type() const {
    return MessageType::PINSTATE;
  }

  std::shared_ptr<Message> PinStateMessage::create(const msgstring &_msg) {
    return std::make_shared<PinStateMessage>(_msg);
  }
}
