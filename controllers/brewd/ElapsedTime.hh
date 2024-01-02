
#ifndef AEGIR_ELAPSEDTIME
#define AEGIR_ELAPSEDTIME

#include <chrono>
#include <string>

namespace aegir {

  class ElapsedTime {
    ElapsedTime() = delete;
    ElapsedTime(const ElapsedTime&) = delete;
    ElapsedTime(ElapsedTime &&) = delete;
    ElapsedTime &operator=(ElapsedTime &&) = delete;
    ElapsedTime &operator=(const ElapsedTime &) = delete;

  public:
    ElapsedTime(std::string _message);
    ~ElapsedTime();

  private:
    std::string c_message;
    std::chrono::time_point<std::chrono::high_resolution_clock> c_start;
  };
}

#endif
