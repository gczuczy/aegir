#include <catch2/catch_session.hpp>

#include "common/logging.hh"

int main(int argc, char* argv[]) {
  aegir::logging::init(false);

  int result = Catch::Session().run( argc, argv );

  return result;
}
