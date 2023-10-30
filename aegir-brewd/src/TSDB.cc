/*
  Time-Series Database
  for storing sensor readings
 */

#include <unistd.h>
#include <stdio.h>

#include <cstdlib>
#include <cstring>
#include <algorithm>

#include "TSDB.hh"

namespace aegir {
  TSDB::TSDB(): c_db(0), c_size(0) {
    // default alloc
    c_alloc_size = TSDB_DEFAULT_SIZE;
    c_db = (entry*)std::malloc(c_alloc_size);
    c_capacity = c_alloc_size / sizeof(entry);
    clear();
  }

  TSDB::~TSDB() {
    if ( c_db ) std::free(c_db);
  }

  const TSDB::entry TSDB::operator[](int i) const {
    entry e;
    return e;
  }

  int TSDB::insert(const datapoints& _data) {
    if ( c_size +1 >= c_capacity ) grow();

    struct timeval tv;
    int idx;

    gettimeofday(&tv, 0);

    {
      std::shared_lock l(c_mutex);
      idx = c_size; // 0-indexed, size is the next entry
      c_db[idx].time = tv.tv_sec;
      c_db[idx].dt = tv.tv_sec - c_db[0].time;

      c_db[idx].readings[0] = _data[0];
      for (int i=0; i< (int)ThermoCouple::_SIZE; ++i)
	c_db[idx].readings[i] = _data[i];

      // by incrementing the size afterwards
      // the new data is made available once it's filled
      // in a shared-lock manner
      ++c_size;
    }
    return idx;
  }

  void TSDB::last(datapoints& _data) {
    std::shared_lock l(c_mutex);
    for (int i=0; i< (int)ThermoCouple::_SIZE; ++i)
      _data[i] = c_db[c_size-1].readings[i];
  }

  void TSDB::at(const uint32_t _idx, entry& _data) const {
    if ( _idx >= c_size )
      throw Exception("TSDB::at idx:%lu vs size:%lu", _idx, 0+c_size);
    {
      std::shared_lock l(c_mutex);
      _data.time = c_db[_idx].time;
      _data.dt = c_db[_idx].dt;
      for (int i=0; i< (int)ThermoCouple::_SIZE; ++i)
	_data.readings[i] = c_db[_idx].readings[i];
    }
  }

  uint32_t TSDB::from(const uint32_t _idx, entry *_data, const uint32_t _size) const {
    uint32_t ncopied(0);
    if ( _idx >= c_size )
      throw Exception("TSDB::from idx:%lu vs size:%lu", _idx, 0+c_size);

    std::shared_lock l(c_mutex);
    uint32_t last = c_size-1;
    ncopied = std::min(_size, last-_idx);

    std::memcpy((void*)_data,
		(void*)(c_db+_idx),
		ncopied * sizeof(entry));

    return ncopied;
  }

  void TSDB::clear() {
    std::memset((void*)c_db, 0, c_alloc_size);
    c_size = 0;
  }

  void TSDB::grow() {
    std::unique_lock l(c_mutex);
    uint32_t prevsize = c_alloc_size;
    uint32_t increment = getpagesize();
    c_alloc_size += increment;
    c_db = (entry*)std::realloc((void*)c_db, c_alloc_size);
    c_capacity = c_alloc_size / sizeof(entry);

    // and finally zero the newly allocated region
    void *start = (void*)(((uint8_t*)c_db)+prevsize);
    std::memset(start, 0, increment);
  }
}
