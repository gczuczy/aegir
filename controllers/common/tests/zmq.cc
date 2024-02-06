
#include "common/ZMQ.hh"

#include <string.h>

#include <cstdint>
#include <thread>
#include <atomic>
#include <chrono>

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

    addProxy("test",
	     "reqa", false,
	     "reqb", true);

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
  auto tmsg = dstmsg->as<TestMessage>();
  REQUIRE(srcmsg.data() == tmsg->data());
}

TEST_CASE("reqrep", "[common][zmq]") {
  int reqval = 42;
  int repval = 69;
  auto cfg = TestConfig::getInstance();

  auto req = cfg->srcSocket("reqrep");
  auto rep = cfg->dstSocket("reqrep");

  req->setSendTimeout(100);
  req->setRecvTimeout(100);
  rep->setSendTimeout(100);
  rep->setRecvTimeout(100);

  req->brrr();
  rep->brrr();

  auto reqmsg = TestMessage(reqval);
  req->send(reqmsg, true);

  {
    auto msg = rep->recv(true);
    REQUIRE(msg != nullptr);
    REQUIRE(reqmsg.group() == msg->group());
    REQUIRE(reqmsg.type() == msg->type());
    REQUIRE(reqmsg.size() == msg->size());

    auto tmsg = msg->as<TestMessage>();
    REQUIRE(tmsg->data() == reqval);

    rep->send(TestMessage(repval), true);
  }

  // receive the response
  {
    auto msg = req->recv(true);
    REQUIRE(msg->group() == TestMessage::msg_group);
    REQUIRE(msg->type() == TestMessage::msg_type);

    auto tmsg = msg->as<TestMessage>();
    REQUIRE(tmsg->data() == repval);
  }
}

class ProxyThread {
public:
  ProxyThread(aegir::zmqproxy_type _proxy): c_proxy(_proxy),
					    c_run(true), c_running(false) {
  };
  ProxyThread()=delete;
  ProxyThread(const ProxyThread&)=delete;
  ProxyThread(ProxyThread&&)=delete;
  ~ProxyThread() {};

  inline bool isRunning() const { return c_running; };

  void stop() {
    c_run = false;
    c_proxy->terminate();

    for (int i=0; i<1000; ++i) {
      if ( !c_running ) break;
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if ( c_running )
      throw std::runtime_error("Proxy did not stop");
    c_thr->join();
  };

  void run() {
    c_thr = std::shared_ptr<std::thread>{new std::thread(&ProxyThread::runProxy,
							 this)};
  };

private:
  void runProxy() {
    c_running = true;
    while ( c_run ) c_proxy->run();
    c_running = false;
  };

private:
  aegir::zmqproxy_type c_proxy;
  std::atomic<bool> c_run, c_running;;
  std::shared_ptr<std::thread> c_thr;
};

TEST_CASE("proxy", "[common][zmq]") {
  int reqval = 42;
  int repval = 69;
  auto cfg = TestConfig::getInstance();
  auto proxy = cfg->proxy("test");
  auto req = cfg->srcSocket("reqa");
  auto rep = cfg->dstSocket("reqb");

  req->setSendTimeout(100);
  req->setRecvTimeout(100);
  rep->setSendTimeout(100);
  rep->setRecvTimeout(100);

  req->brrr();
  rep->brrr();

  ProxyThread pthr(proxy);
  pthr.run();

  auto reqmsg = TestMessage(reqval);
  req->send(reqmsg, true);

  {
    auto msg = rep->recv(true);
    REQUIRE(msg != nullptr);
    REQUIRE(msg->group() == TestMessage::msg_group);
    REQUIRE(msg->type() == TestMessage::msg_type);

    auto tmsg = msg->as<TestMessage>();
    REQUIRE(tmsg->data() == reqval);

    rep->send(TestMessage(repval), true);
  }

  // receive the response
  {
    auto msg = req->recv(true);
    REQUIRE(msg->group() == TestMessage::msg_group);
    REQUIRE(msg->type() == TestMessage::msg_type);

    auto tmsg = msg->as<TestMessage>();
    REQUIRE(tmsg->data() == repval);
  }

  pthr.stop();
}
