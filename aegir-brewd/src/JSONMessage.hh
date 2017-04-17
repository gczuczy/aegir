/*
 * This class implements the JSON Message format
 */

#ifndef AEGIR_JSONMESSAGE_H
#define AEGIR_JSONMESSAGE_H

#include <jsoncpp/json/json.h>

#include "Message.hh"

namespace aegir {

  class JSONMessage: public Message {
  public:
    JSONMessage(const msgstring &_msg);
    JSONMessage(const Json::Value &_json);
    virtual ~JSONMessage();
    virtual msgstring serialize() const override;
    virtual MessageType type() const override;
    inline const Json::Value &getJSON() const { return c_json; };

  private:
    Json::Value c_json;
  };
}

#endif
