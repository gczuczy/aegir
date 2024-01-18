#include <stdio.h>
#include <unistd.h>
#include <cstdlib>
#include <set>

#include "TiltDB.hh"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("TiltDB ctor", "[TiltDB]") {
  auto db = aegir::fermd::TiltDB::getInstance();

  REQUIRE(db != 0);

  std::set<std::string> uuids{"a", "b"};

  auto tilts = db->getEntries();
#if 0
  for (auto it: tilts) printf("%s %s %s\n", it.strUUID().c_str(),
			      it.color,
			      it.active ? "active" : "inactive");
#endif
}
