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
    typedef float datapoints[(size_t)ThermoCouple::_SIZE];
    struct alignas(sizeof(long)) entry {
      time_t time;
      time_t dt;
      datapoints readings;
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

    int insert(const datapoints& _data);
    void last(datapoints& _data);
    void at(const uint32_t _idx, entry& _data) const;
    uint32_t from(const uint32_t _idx, entry *_data, const uint32_t _size) const;
    inline uint32_t size() const {return c_size;};

  private:
    void clear();
    void grow();
  };
}
