/*
 * This class holds the current process state
 * - Stores the program
 * - Holds sensor state information
 * - Holds sensor statistics info
 */

#ifndef AEGIR_PROCESSSTATE_H
#define AEGIR_PROCESSSTATE_H

#include <memory>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <map>
#include <set>
#include <string>

#include "Program.hh"

namespace aegir {

  class ProcessState {
  public:
    enum class States: uint8_t {
      Empty=0, // initialized, no program loadad
	Loaded, // program loaded, but not started
	PreWait, // timed mode, waiting for pre-heat start
	PreHeat, // pre-heating to starttemp
	Mashing, // Doing the mash steps
	Sparging, // keeps on endtemp temperature, and circulates
	PreBoil, // Heats the BK up to boiling, till the start of the boil timer
	Hopping, // BK being boild, hopping timers started
	Cooling, // The wort is being cooled down, later whirlpool and such
	Finished // Brewind process finished
    };
  private:
    ProcessState();
    ProcessState(ProcessState&&) = delete;
    ProcessState(const ProcessState &) = delete;
    ProcessState &operator=(ProcessState&&) = delete;
    ProcessState &operator=(const ProcessState &) = delete;
  public:
    ~ProcessState();
    static ProcessState &getInstance();
    ProcessState &reconfigure();

    bool isActive() const;
    ProcessState &loadProgram(const Program &_prog, uint32_t _startat, uint32_t _volume);
    std::shared_ptr<Program> getProgram();
    inline States getState() const { return c_state; };
    std::string getStringState() const;
    ProcessState &addThermoReading(const std::string &_sensor, const uint32_t _time, const double _temp);
    ProcessState &getThermoCouples(std::set<std::string> &_tcs);
    ProcessState &getTCReadings(const std::string &_sensor, std::map<uint32_t, double> &_tcvals);

  private:
    // inputs
    std::shared_ptr<Program> c_program;
    uint32_t c_startat;
    uint32_t c_volume; //liters
    // states
    std::atomic<States> c_state;
    std::recursive_mutex c_mtx_state;
    std::map<std::string, std::map<uint32_t, double> > c_thermoreadings;
  };
}

#endif
