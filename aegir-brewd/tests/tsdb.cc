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
  aegir::ThermoReadings e,f;
  aegir::TSDB::entry g;

  uint32_t idx;

  INFO("TSDB_DEFAULT_SIZE: " << TSDB_DEFAULT_SIZE);
  int test_entries = (TSDB_DEFAULT_SIZE*1.3)/sizeof(aegir::TSDB::entry);
  INFO("test_entries: " << test_entries);
  INFO("test size: "<< (test_entries * sizeof(aegir::TSDB::entry)));
  for (int i=0; i < test_entries; ++i) {
    e.data[0] = 1.0f * i;
    e.data[1] = 1.5f * i;
    e.data[2] = 2.0f * i;
    e.data[3] = 2.5f * i;
    idx = db.insert(e);
  }

  INFO("Inserted: "<< db.size());
  REQUIRE(db.size() == test_entries);

  f = db.last();
  REQUIRE(e.data[0] == f.data[0]);
  REQUIRE(e.data[1] == f.data[1]);
  REQUIRE(e.data[2] == f.data[2]);
  REQUIRE(e.data[3] == f.data[3]);

  db.at(idx-1, g);
  REQUIRE(g.readings.data[0] == 1.0f*(idx-1));
  REQUIRE(g.readings.data[1] == 1.5f*(idx-1));
  REQUIRE(g.readings.data[2] == 2.0f*(idx-1));
  REQUIRE(g.readings.data[3] == 2.5f*(idx-1));
}

TEST_CASE("TSDB from", "[TSDB]") {
  aegir::TSDB db;
  aegir::ThermoReadings e,f;
  aegir::TSDB::entry g;

  uint32_t idx;

  INFO("TSDB_DEFAULT_SIZE: " << TSDB_DEFAULT_SIZE);
  INFO("TSDB entry size: " << sizeof(aegir::TSDB::entry));
  int test_entries = (TSDB_DEFAULT_SIZE*1.3)/sizeof(aegir::TSDB::entry);
  INFO("test_entries: " << test_entries);
  INFO("test size: "<< (test_entries * sizeof(aegir::TSDB::entry)));
  for (int i=0; i < test_entries; ++i) {
    e.data[0] = 1.0f * i;
    e.data[1] = 1.5f * i;
    e.data[2] = 2.0f * i;
    e.data[3] = 2.5f * i;
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
