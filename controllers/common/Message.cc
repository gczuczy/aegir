
#include "common/Message.hh"

#include <stdlib.h>
#include <string.h>

#include "common/Exception.hh"
#include "common/misc.hh"

namespace aegir {

  Message::Message(MessageGroup _group, std::uint8_t _type,
		   std::uint16_t _size): c_buffer(0) {
    c_buffer = (void*)malloc(_size);
    c_headers = (headers*)c_buffer;

    c_headers->group = _group;
    c_headers->type = _type;
    c_headers->size = _size;

    setPointers();
  }

  Message::Message(const void* _buffer): c_buffer(0) {
    resize(((headers*)_buffer)->size);
    memcpy(c_buffer, _buffer, ((headers*)_buffer)->size);
    c_headers = (headers*)c_buffer;
  }

  Message::~Message() {
    if ( c_buffer ) free(c_buffer);
  }

  void Message::resize(std::uint16_t _size) {
    c_buffer = (void*)realloc((void*)c_buffer, _size);
    c_headers = (headers*)c_buffer;
  }

  void Message::setPointers() {
  }

  /*
    RawMessage
   */
  RawMessage::RawMessage(const void* _data,
			 std::uint16_t _size,
			 bool _copy): Message(MessageGroup::CORE,
					      (uint8_t)MessageType::RAW),
				      c_iscopy(_copy) {
    if ( _copy ) {
      c_data = malloc(_size);
      memcpy(c_data, _data, _size);
    } else {
      c_data = (void*)_data;
    }
    c_headers->size = _size;
  }

  RawMessage::~RawMessage() {
    if ( c_iscopy ) free(c_data);
  }

  const void* RawMessage::serialize() const {
    return c_data;
  }

  /*
    MessageFactory
   */
  MessageFactory::MessageFactory() noexcept {
  }

  MessageFactory::~MessageFactory() {
  }

  void MessageFactory::bailout() {
    c_handlers.clear();
  }

  message_type MessageFactory::parse(const char* _buffer, std::uint16_t _len) {
    if ( _len < sizeof(Message::headers) )
      throw Exception("MessageFactory::parse(): buffer too small");
    Message::headers *hdr = (Message::headers*)_buffer;

    if ( hdr->size != _len )
      throw Exception("MessageFactory::parse(): buffer size mismatch");

    auto git = c_handlers.find(hdr->group);
    if ( git == c_handlers.end() )
      throw Exception("Group not found");

    auto tit = git->second.find(hdr->type);
    if ( tit == git->second.end() )
      throw Exception("No handler for %u/%u",
		      hdr->group, hdr->type);

    return tit->second(_buffer);
  }
}
