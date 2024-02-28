
#include <stdlib.h>

#include <exception>

#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

#include "common/ryml.hh"

class eventRunListener : public Catch::EventListenerBase {
public:
  using Catch::EventListenerBase::EventListenerBase;

  void testRunStarting(Catch::TestRunInfo const&) override {
    aegir::init_ryml();
    srand(time(0));
    std::srand(std::time(0));
  }
};

CATCH_REGISTER_LISTENER(eventRunListener);
