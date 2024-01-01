/*
  This singleton holds the state of the environment, which is
  independent from an active brew process
 */

#ifndef AEGIR_ENVIRONMENT_H
#define AEGIR_ENVIRONMENT_H

#include <memory>
#include <atomic>

#include "types.hh"

namespace aegir {

  class Environment {
  private:
    Environment();

  public:
    ~Environment();
    Environment(const Environment&) = delete;
    Environment(Environment&&) = delete;

    static std::shared_ptr<Environment> getInstance();

    void setThermoReadings(ThermoReadings& _r);
    inline float getTempMT() const { return c_temp_mt; };
    inline float getTempRIMS() const { return c_temp_rims; };
    inline float getTempBK() const { return c_temp_bk; };
    inline float getTempHLT() const { return c_temp_hlt; };

  private:
    std::atomic<float> c_temp_mt;
    std::atomic<float> c_temp_rims;
    std::atomic<float> c_temp_bk;
    std::atomic<float> c_temp_hlt;
  };
}

#endif
