/*
  ThreadPool
 */

#include "common/Message.hh"

#include <string.h>

#include <cstdint>

#include "common/misc.hh"

#include <catch2/catch_test_macros.hpp>

class TestMessage: public aegir::Message {
public:
  static constexpr aegir::MessageGroup msg_group = aegir::MessageGroup::CORE;
  static constexpr std::uint8_t msg_type = 42;

public:
  TestMessage(int _data): Message(msg_group, msg_type, sizeof(headers)+sizeof(int)) {
    //resize(size()+sizeof(int));
    c_data = dataPtr<int>();
    c_headers->size = sizeof(headers)+sizeof(int);
    *c_data = _data;
  }
  virtual ~TestMessage() {};
private:
  TestMessage(const char* _buffer): Message(_buffer)  {
    c_data = dataPtr<int>();
  }

public:
  static aegir::message_type parse(const char* _buffer) {
    return std::shared_ptr<TestMessage>{new TestMessage(_buffer)};
  };
  int data() const { return *c_data; };

private:
  int *c_data;

};

TEST_CASE("Message", "[common]") {
  auto f = aegir::MessageFactory::getInstance();

  f->registerHandler<TestMessage>();

  struct testdata_t {
    aegir::Message::headers headers;
    int data;
  } testdata;

  testdata.headers.group = TestMessage::msg_group;
  testdata.headers.type = TestMessage::msg_type;
  testdata.headers.size = sizeof(testdata);
  testdata.data = 42;
  TestMessage testmsg(testdata.data);
  void *ts = testmsg.serialize();
  aegir::Message::headers* h = (aegir::Message::headers*)ts;

  REQUIRE(h->size == testdata.headers.size);
  REQUIRE(memcmp((const char*)&testdata,
		 (const char*)ts,
		 testmsg.size())==0);

  auto msg = f->parse((const char*)&testdata, testdata.headers.size);
  REQUIRE(msg->group() == TestMessage::msg_group);
  REQUIRE(msg->type() == TestMessage::msg_type);
  REQUIRE(msg->size() == testdata.headers.size);

  auto tmsg = msg->as<TestMessage>();
  REQUIRE(tmsg-> data() == testdata.data);
}
