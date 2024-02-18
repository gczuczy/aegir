/*
  fermd-specific messages
 */

#ifndef AEGIR_FERMD_H
#define AEGIR_FERMD_H

#include <time.h>

#include "common/Message.hh"
#include "fermd/DB.hh"

namespace aegir {
  namespace fermd {

    enum class MessageType: std::uint8_t {
      TiltReading=0
    };

    /*
      Fermd message factory
     */

    /*
      Tilt Reading
     */
    class TiltReadingMessage: public aegir::Message {
    public:
      static constexpr aegir::MessageGroup msg_group = aegir::MessageGroup::FERMD;
      static constexpr std::uint8_t msg_type = (std::uint8_t)MessageType::TiltReading;

    private:
      static constexpr std::uint16_t uuid_offset = 0;
      static constexpr std::uint16_t time_offset = uuid_offset + sizeof(uuid_t);
      static constexpr std::uint16_t temp_offset = time_offset + sizeof(time_t);
      static constexpr std::uint16_t sg_offset   = temp_offset + sizeof(float);
      static constexpr std::uint16_t total_size  = sizeof(headers) + sg_offset + sizeof(float);

    public:
      TiltReadingMessage(uuid_t &_uuid, time_t time, float _temp, float _sg);
      virtual ~TiltReadingMessage();
    private:
      TiltReadingMessage()=delete;
      TiltReadingMessage(const char* _buff);
      TiltReadingMessage(const TiltReadingMessage&)=delete;
      TiltReadingMessage(TiltReadingMessage&&)=delete;

    protected:
      virtual void setPointers();

    public:
      inline uuid_t uuid() const noexcept { return *c_uuid; };
      inline time_t time() const noexcept { return *c_time; };
      inline float temp() const noexcept { return *c_temp; };
      inline float sg() const noexcept { return *c_sg; };

      static aegir::message_type parse(const char*);
      static aegir::message_type create(uuid_t _uuid,
					time_t _time,
					float _temp,
					float _sg);

    private:
      uuid_t *c_uuid;
      time_t *c_time;
      float *c_temp;
      float *c_sg;
    };
  }
}

#endif
