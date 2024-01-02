/*
  Time-Series DataBase
  Storing temperature readings across sensors
 */

#include <sys/time.h>
#include <cstdint>
#include <atomic>
#include <shared_mutex>
#include <ostream>

#include "types.hh"
#include "Exception.hh"

#define TSDB_DEFAULT_SIZE (16*1024)

namespace aegir {

  class TSDB {
  public:
    // types
    struct alignas(sizeof(long)) entry {
      time_t time;
      time_t dt;
      ThermoReadings readings;
      inline float& operator[](std::size_t _idx) {return readings[_idx];};
      inline const float& operator[](std::size_t _idx) const {return readings[_idx];};
      friend std::ostream& operator<<(std::ostream& os, const TSDB::entry& e) {
	return os << "TSDB::entry(" << e.time << ", " << e.dt << ", " << e.readings << ")";
      };
    };
    struct Iterator {
      Iterator()=delete;
      Iterator(const TSDB& _db, uint32_t _index=0);
      ~Iterator();

      inline uint32_t index() const {return c_index; };
      inline const entry& operator*() { return c_db[c_index]; };
      inline const entry* operator->() { return &(c_db[c_index]); };
      friend std::ostream& operator<<(std::ostream& os, const Iterator& i) {
	return os << "TSDB::Iterator(" << i.c_index << ", " << i.c_db[i.c_index] << ")";
      };
    private:
      const TSDB& c_db;
      uint32_t c_index;
    };

  private:
    entry *c_db;
    uint32_t c_alloc_size; // allocated size in bytes
    uint32_t c_capacity; // size in number of entries
    std::atomic<uint32_t> c_size;
    mutable std::shared_mutex c_mutex;

  public:
    TSDB();
    TSDB(TSDB&) = delete;
    TSDB(const TSDB&) = delete;
    TSDB(TSDB&&) = delete;
    ~TSDB();

    const entry& operator[](int i) const;

    inline void lock_shared() const { c_mutex.lock_shared(); };
    inline void unlock_shared() const { c_mutex.unlock_shared(); };

    inline int insert(const ThermoReadings& _data) {
      struct timeval tv;
      gettimeofday(&tv, 0);

      return insert(tv.tv_sec, _data);
    }
    int insert(const time_t _time, const ThermoReadings& _data);
    const entry last() const;
    const entry at(const uint32_t _idx) const;
    Iterator atTime(const uint32_t _time) const;
    Iterator atDeltaTime(const uint32_t _dt) const;
    uint32_t from(const uint32_t _idx, entry *_data, const uint32_t _size) const;
    inline uint32_t size() const {return c_size;};
    time_t getStartTime() const;
    inline Iterator begin() const {return Iterator(*this, 0); };
    void clear();

  private:
    uint32_t lookup(time_t _time) const;
    void grow();
  };

}
