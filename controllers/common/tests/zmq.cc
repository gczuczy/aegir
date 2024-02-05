
#include "common/ZMQ.hh"

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
    setPointers();
    c_headers->size = sizeof(headers)+sizeof(int);
    *c_data = _data;
  }
  virtual ~TestMessage() {};
private:
  TestMessage(const char* _buffer): Message(_buffer)  {
    setPointers();
  }

public:
  static aegir::message_type parse(const char* _buffer) {
    return std::shared_ptr<TestMessage>{new TestMessage(_buffer)};
  };
  inline int data() const { return *c_data; };

public:
  virtual void setPointers() {
    setPointer(c_data, 0);
  }

private:
  int *c_data;

};

class TestConfig: public aegir::ZMQConfig {
private:
  TestConfig(): aegir::ZMQConfig() {
    // simple tests
    addSpec("reqrep",
	    aegir::ZMQConfig::zmq_proto::INPROC,
	    ZMQ_REQ, ZMQ_REP,
	    "reqrep", 0);
    addSpec("pubsub",
	    aegir::ZMQConfig::zmq_proto::INPROC,
	    ZMQ_PUB, ZMQ_SUB,
	    "pubsub", 0, true);

    // proxy
    addSpec("reqa",
	    aegir::ZMQConfig::zmq_proto::INPROC,
	    ZMQ_REQ, ZMQ_ROUTER,
	    "reqa", 0,
	    false);
    addSpec("reqb",
	    aegir::ZMQConfig::zmq_proto::INPROC,
	    ZMQ_DEALER, ZMQ_REP,
	    "reqb", 0,
	    true);
    addSpec("reqctrl",
	    aegir::ZMQConfig::zmq_proto::INPROC,
	    ZMQ_PUB, ZMQ_SUB,
	    "reqctrl", 0,
	    false);

    addProxy("test",
	     "reqa", false,
	     "reqb", true,
	     "reqctrl", false);

    // our message type
    auto msf = aegir::MessageFactory::getInstance();

    msf->registerHandler<TestMessage>();
  }
public:
  TestConfig(const TestConfig&)=delete;
  TestConfig(TestConfig&&)=delete;
  virtual ~TestConfig() {};
  static std::shared_ptr<TestConfig> getInstance() {
    static std::shared_ptr<TestConfig> instance{new TestConfig()};
    return instance;
  }
};

TEST_CASE("pubsub", "[common][zmq]") {
  auto cfg = TestConfig::getInstance();

  auto src = cfg->srcSocket("pubsub");
  auto dst = cfg->dstSocket("pubsub");

  src->setSendTimeout(100);
  dst->setRecvTimeout(100);
  dst->subscribe("");

  src->brrr();
  dst->brrr();

  auto srcmsg = TestMessage(42);
  src->send(srcmsg, true);

  auto dstmsg = dst->recv(true);
  REQUIRE(dstmsg != nullptr);
  REQUIRE(srcmsg.group() == dstmsg->group());
  REQUIRE(srcmsg.type() == dstmsg->type());
  REQUIRE(srcmsg.size() == dstmsg->size());
}
