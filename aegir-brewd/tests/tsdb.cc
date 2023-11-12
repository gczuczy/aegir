#include <stdio.h>
#include <unistd.h>
#include <cstdlib>
#include <set>

#include "TSDB.hh"

#include <catch2/catch_test_macros.hpp>

void init() {
  std::srand(std::time(0));
}

float randf(const float _min, const float _max) {
  float r = static_cast<float>(std::rand() / static_cast<float>(RAND_MAX));

  return _min + (_max - _min)*r;
}

void fill(aegir::TSDB& _db, uint32_t _n, const std::set<time_t>& _gaps = {}) {
  time_t now = time(0);
  aegir::ThermoReadings tr;
  bool skipped=false;

  for (time_t t=now-_n; t<=now; ++t) {
    time_t gapdt=_n-(now-t);
    if ( _gaps.find(gapdt) != _gaps.end() ) {
      skipped=true;
      continue;
    }
    for (std::size_t i=0; i<aegir::ThermoCouple::_SIZE; ++i)
      tr[i] = randf(25.0, 80.0);
    int idx = _db.insert(t, tr);
    skipped=false;
  }
}

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
  aegir::ThermoReadings e;
  aegir::TSDB::entry g, f;

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
  REQUIRE(e.data[0] == f[0]);
  REQUIRE(e.data[1] == f[1]);
  REQUIRE(e.data[2] == f[2]);
  REQUIRE(e.data[3] == f[3]);

  g = db.at(idx-1);
  REQUIRE(g[0] == 1.0f*(idx-1));
  REQUIRE(g[1] == 1.5f*(idx-1));
  REQUIRE(g[2] == 2.0f*(idx-1));
  REQUIRE(g[3] == 2.5f*(idx-1));
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

time_t foobar(time_t _x) {
  return _x;
}

TEST_CASE("TSDB::Iterator", "[TSDB]") {
  // fill with data
  time_t now = time(0);
  uint32_t duration = 7200;
  std::set<time_t> gaps{42, 69, 418, 1234};
  aegir::TSDB db;

  init();

  fill(db, duration, gaps);

  // gaps
  for (auto g: gaps) {
    INFO("g: " << g);
    auto it = db.atTime(now-duration+g);
    INFO("it: " << it);
    REQUIRE( (g < it->dt? it->dt - g : g - it->dt) == 1);
  }
}
