/*
  Time-Series DataBase
  Storing temperature readings across sensors
 */

#include <cstdint>
#include <atomic>
#include <sys/time.h>
#include <shared_mutex>

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

    const entry operator[](int i) const;

    inline int insert(const ThermoReadings& _data) {
      struct timeval tv;
      gettimeofday(&tv, 0);

      return insert(tv.tv_sec, _data);
    }
    int insert(const time_t _time, const ThermoReadings& _data);
    const ThermoReadings& last() const;
    void at(const uint32_t _idx, entry& _data) const;
    uint32_t from(const uint32_t _idx, entry *_data, const uint32_t _size) const;
    inline uint32_t size() const {return c_size;};

  private:
    void clear();
    void grow();
  };
}
