
#include "JSONMessage.hh"
#include "Exception.hh"

#include <string.h>
#include <stdio.h>

#include <sstream>

namespace aegir {
  JSONMessage::JSONMessage(const msgstring &_msg) {
    //    if ( !jsr.parse(msg, c_json, false) )
    Json::CharReaderBuilder crf;
    std::string msg;
    msg.resize(_msg.size());
    memcpy((void*)msg.data(), (void*)_msg.data(), msg.size());

    std::istringstream iss(msg);
    std::string errors;
    if ( Json::parseFromStream(crf, iss, &c_json, &errors) )
      throw Exception("Cannot parse message as JSON: %s", errors.c_str());

#if 0
    std::string typestr("unknown");
    switch(c_json.type()) {
    case Json::ValueType::nullValue:
      typestr = "null";
      break;
    case Json::ValueType::intValue:
      typestr = "int";
      break;
    case Json::ValueType::uintValue:
      typestr = "uint";
      break;
    case Json::ValueType::realValue:
      typestr = "real";
      break;
    case Json::ValueType::stringValue:
      typestr = "string";
      break;
    case Json::ValueType::booleanValue:
      typestr = "boolean";
      break;
    case Json::ValueType::arrayValue:
      typestr = "array";
      break;
    case Json::ValueType::objectValue:
      typestr = "object";
      break;
    }
    printf("JSONMessage(): parsed input's type: %s\nsrc: %s\n", typestr.c_str(), _msg.c_str());
#endif
  }

  JSONMessage::JSONMessage(const Json::Value &_json): c_json(_json) {
  }

  JSONMessage::~JSONMessage() = default;

  msgstring JSONMessage::serialize() const {
    Json::StreamWriterBuilder swb;
    std::string buff;
    msgstring msg;

    buff = Json::writeString(swb, c_json);

    msg.resize(buff.size());
    memcpy((void*)msg.data(), (void*)buff.data(), msg.size());

    return msg;
  }

  MessageType JSONMessage::type() const {
    return MessageType::JSON;
  }

}
