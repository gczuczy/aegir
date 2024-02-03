#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <cstdlib>

#include "Message.hh"

#include <catch2/catch_test_macros.hpp>

void init() {
  static bool initialized(false);
  if ( !initialized ) {
    std::srand(std::time(0));
    initialized = true;
  }
}

TEST_CASE("TiltReadingMessage", "[fermd][message]") {
  struct alignas(int) trm_t {
    aegir::Message::headers headers;
    aegir::fermd::uuid_t uuid;
    float temp;
    float sg;
  } testdata;

  // fill the testdata with random
  for (int i=0; i<sizeof(trm_t); i+=sizeof(int)) {
    assert(i+sizeof(int) <= sizeof(trm_t));
    char* buff = ((char*)&testdata)+i;
    *((int*)buff) = std::rand();
  }
  testdata.headers.group = aegir::fermd::TiltReadingMessage::msg_group;
  testdata.headers.type = aegir::fermd::TiltReadingMessage::msg_type;
  testdata.headers.size = sizeof(trm_t);

  char buffer[128];
  // testing data ctor
  {
    auto msg = aegir::fermd::TiltReadingMessage(testdata.uuid,
						testdata.temp,
						testdata.sg);
    REQUIRE(msg.uuid() == testdata.uuid);
    REQUIRE(msg.temp() == testdata.temp);
    REQUIRE(msg.sg() == testdata.sg);
    REQUIRE(memcmp((void*)msg.serialize(),
		   (void*)&testdata,
		   msg.size())==0);

    // unmarshalling it
    auto msg2 = aegir::fermd::TiltReadingMessage::parse((char*)msg.serialize());

    REQUIRE(msg.group() == msg2->group());
    REQUIRE(msg.type() == msg2->type());
    auto msg3 = msg2->as<aegir::fermd::TiltReadingMessage>();
    REQUIRE(msg.uuid() == msg3->uuid());
    REQUIRE(msg.temp() == msg3->temp());
    REQUIRE(msg.sg() == msg3->sg());
  }

  // unmarshalling
}
