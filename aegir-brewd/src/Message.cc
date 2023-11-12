
#include "Message.hh"

#include <string.h>
#include <stdio.h>

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
  MessageFactoryReg ThermoReadingReg(MessageType::THERMOREADING, ThermoReadingMessage::create);

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
   * cycletime: 4 bytes (sizeof float)
   * onratio: 4 bytes (sizeof float)
   */
  PinStateMessage::PinStateMessage(const msgstring &_msg) {
#ifdef AEGIR_DEBUG
    printf("PinStateMessage(L:%lu '%s') called: ", _msg.length(), hexdump(_msg).c_str());
#endif
    // _msg[0] is the type, but we're already here
    int msglen = _msg.length();
    uint32_t offset = 1;

    uint8_t namelen = _msg[offset++];
    c_name = std::string((char*)_msg.data()+offset, namelen);
    offset += namelen;

    c_state = (PINState)(_msg[offset++]);

    c_cycletime = *((float*)((char*)_msg.data()+offset));
    offset += 4;

    c_onratio = *((float*)((char*)_msg.data()+offset));
#ifdef AEGIR_DEBUG
    printf(" name:'%s' state:%hhu CT:%.3f OR:%.3f\n", c_name.c_str(), (uint8_t)c_state,
	   c_cycletime, c_onratio);
#endif
  }

  PinStateMessage::PinStateMessage(const std::string &_name, PINState _state, float _cycletime, float _onratio):
    c_name(_name), c_state(_state), c_cycletime(_cycletime), c_onratio(_onratio) {
#ifdef AEGIR_DEBUG
    printf("PinStateMessage('%s', %hhu, %.3f, %.3f) called: ", _name.c_str(), (uint8_t)_state,
	   _cycletime, _onratio);
#endif
    // cycletime checks
    if ( _state == PINState::Pulsate ) {
      if ( _cycletime < 0.2f ) throw Exception("PinStateMessage() cycletime must be greater than 0.2");
      if ( _cycletime > 10.0f ) throw Exception("PinStateMessage() cycletime must be less than 10");
    // on ratio checks
      if ( _onratio < 0.0f ) c_state = PINState::Off;
      if ( _onratio > 1.0f ) c_state = PINState::On;
    }

#ifdef AEGIR_DEBUG
    printf("'%s'\n", hexdump(serialize()).c_str());
#endif
  }

  PinStateMessage::~PinStateMessage() {
  }

  msgstring PinStateMessage::serialize() const {
    uint8_t strsize(std::min((uint32_t)c_name.length(), (uint32_t)255));
    uint32_t len(3+8+strsize);

    msgstring buffer(len, 0);
    uint8_t *data = (uint8_t*)buffer.c_str();
    data[0] = (uint8_t)type();
    data[1] = (uint8_t)strsize;
    int offset = 2;

    memcpy((void*)(data+2), (void*)c_name.data(), strsize);
    offset += strsize;
    data[offset++] = (int8_t)c_state;

    *(float*)(data+offset) = c_cycletime;

    offset += 4;
    *(float*)(data+offset) = c_onratio;

    return buffer;
  }

  MessageType PinStateMessage::type() const {
    return MessageType::PINSTATE;
  }

  std::shared_ptr<Message> PinStateMessage::create(const msgstring &_msg) {
    return std::make_shared<PinStateMessage>(_msg);
  }

  /*
   * ThermoReadingMessage
   * Format is:
   * MessageType: 1 byte
   * Name: 1) stringsize: 1 byte + 2) stringsize-bytes
   * Temp: sizeof(float)
   * Timestamp: 4 byte, uint32_t
   */
  ThermoReadingMessage::ThermoReadingMessage(const msgstring &_msg) {
#ifdef AEGIR_DEBUG
    printf("ThermoReadingMessage(L:%lu '%s') called\n", _msg.length(), hexdump(_msg).c_str());
#endif
    // _msg[0] is the type, but we're already here
    int msglen = _msg.length();
    uint8_t *data = (uint8_t*)_msg.data();
    int offset = 1;

    for (int i=0; i<ThermoCouple::_SIZE; ++i) {
      c_data.data[i] = *(float*)(data+offset);
      offset += sizeof(float);
    }

    c_timestamp = *(uint32_t*)(data+offset);

#ifdef AEGIR_DEBUG
    printf("ThermoReadingMessage(L:%lu '%s') decoded: Name:'%s' Temp:%.2f Time:%u\n", _msg.length(), hexdump(_msg).c_str(),
	   c_name.c_str(), c_temp, c_timestamp);
#endif
  }

  ThermoReadingMessage::ThermoReadingMessage(const ThermoReadings &_data, uint32_t _timestamp):
    c_data(_data), c_timestamp(_timestamp) {
  }

  ThermoReadingMessage::~ThermoReadingMessage() = default;

  msgstring ThermoReadingMessage::serialize() const {
    uint32_t len = 2 + sizeof(c_data) + sizeof(c_timestamp);

    msgstring buffer(len, 0);
    uint8_t *data = (uint8_t*)buffer.data();
    data[0] = (uint8_t)type();

    int offset = 1;
    for (int i=0; i<ThermoCouple::_SIZE; ++i) {
      *(float*)(data+offset) = c_data.data[i];
      offset += sizeof(float);
    }

    // c_timestamp is uint32_t
    *(uint32_t*)(data+offset) = c_timestamp;

    return buffer;
  }

  MessageType ThermoReadingMessage::type() const {
    return MessageType::THERMOREADING;
  }

  std::shared_ptr<Message> ThermoReadingMessage::create(const msgstring &_msg) {
    return std::make_shared<ThermoReadingMessage>(_msg);
  }

}
