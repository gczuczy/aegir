
#include "Message.hh"

namespace aegir {
  namespace fermd {

    /*
      TiltReadingMessage
     */
    TiltReadingMessage::TiltReadingMessage(uuid_t &_uuid, time_t _time,
					   float _temp, float _sg):
      Message(msg_group, msg_type, total_size) {
      setPointers();
      *c_uuid = _uuid;
      *c_time = _time;
      *c_temp = _temp;
      *c_sg = _sg;
    }

    TiltReadingMessage::TiltReadingMessage(const char* _buff): Message(_buff) {
      setPointers();
    }

    TiltReadingMessage::~TiltReadingMessage() {
    }

    void TiltReadingMessage::setPointers() {
      setPointer(c_uuid, uuid_offset);
      setPointer(c_time, time_offset);
      setPointer(c_temp, temp_offset);
      setPointer(c_sg, sg_offset);
    }

    aegir::message_type TiltReadingMessage::parse(const char* _buffer) {
      return std::shared_ptr<TiltReadingMessage>{new TiltReadingMessage(_buffer)};
    }
    aegir::message_type TiltReadingMessage::create(uuid_t _uuid,
						   time_t _time,
						   float _temp,
						   float _sg) {
      return std::shared_ptr<TiltReadingMessage>{new TiltReadingMessage(_uuid,
									_time,
									_temp,
									_sg)};
    }
  }
}
