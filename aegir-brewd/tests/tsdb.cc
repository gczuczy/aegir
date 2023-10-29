#include <stdio.h>
#include <unistd.h>

#include "TSDB.hh"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("TSDB::entry size", "[TSDB]") {
  auto entrysize = sizeof(aegir::TSDB::entry);
  auto longsize = sizeof(long);
  auto n = entrysize / longsize;
  REQUIRE(entrysize == (n*longsize));
}

TEST_CASE("TSDB (de)alloc", "[TSDB]") {
  REQUIRE_NOTHROW(aegir::TSDB());
}

TEST_CASE("TSDB insertion", "[TSDB]") {
  aegir::TSDB db;
  aegir::TSDB::datapoints e,f;
  aegir::TSDB::entry g;

  uint32_t idx;

  INFO("TSDB_DEFAULT_SIZE: " << TSDB_DEFAULT_SIZE);
  int test_entries = (TSDB_DEFAULT_SIZE*1.3)/sizeof(aegir::TSDB::entry);
  INFO("test_entries: " << test_entries);
  INFO("test size: "<< (test_entries * sizeof(aegir::TSDB::entry)));
  for (int i=0; i < test_entries; ++i) {
    e[0] = 1.0f * i;
    e[1] = 1.5f * i;
    e[2] = 2.0f * i;
    e[3] = 2.5f * i;
    idx = db.insert(e);
  }

  INFO("Inserted: "<< db.size());
  REQUIRE(db.size() == test_entries);

  db.last(f);
  REQUIRE(e[0] == f[0]);
  REQUIRE(e[1] == f[1]);
  REQUIRE(e[2] == f[2]);
  REQUIRE(e[3] == f[3]);

  db.at(idx-1, g);
  REQUIRE(g.readings[0] == 1.0f*(idx-1));
  REQUIRE(g.readings[1] == 1.5f*(idx-1));
  REQUIRE(g.readings[2] == 2.0f*(idx-1));
  REQUIRE(g.readings[3] == 2.5f*(idx-1));
}

TEST_CASE("TSDB from", "[TSDB]") {
  aegir::TSDB db;
  aegir::TSDB::datapoints e,f;
  aegir::TSDB::entry g;

  uint32_t idx;

  INFO("TSDB_DEFAULT_SIZE: " << TSDB_DEFAULT_SIZE);
  int test_entries = (TSDB_DEFAULT_SIZE*1.3)/sizeof(aegir::TSDB::entry);
  INFO("test_entries: " << test_entries);
  INFO("test size: "<< (test_entries * sizeof(aegir::TSDB::entry)));
  for (int i=0; i < test_entries; ++i) {
    e[0] = 1.0f * i;
    e[1] = 1.5f * i;
    e[2] = 2.0f * i;
    e[3] = 2.5f * i;
    idx = db.insert(e);
  }

  INFO("Inserted: "<< db.size());

  uint32_t buflen = getpagesize()/sizeof(aegir::TSDB::entry);
  aegir::TSDB::entry buffer[buflen];

  idx = 0;
  try {
    while (db.from(idx, (aegir::TSDB::entry*)&buffer, buflen) == buflen) {
      idx += buflen;
    }
  }
  catch (aegir::Exception &e) {
    INFO("Exception: " << e.what());
  }
}
