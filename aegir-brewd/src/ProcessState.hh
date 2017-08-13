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
#include <list>
#include <string>
#include <functional>

#include "Program.hh"

namespace aegir {

  class ProcessState {
  public:
    class Guard {
    public:
      Guard() = delete;
      Guard(Guard&&) = delete;
      Guard(const Guard &) = delete;
      Guard &operator=(Guard &&) = delete;
      Guard &operator=(const Guard &) = delete;
      Guard(ProcessState &_ps);
      ~Guard();
    private:
      ProcessState &c_ps;
    };
    friend Guard;
    enum class States: uint8_t {
      Empty=0, // initialized, no program loadad
	Loaded, // program loaded, but not started
	PreWait, // timed mode, waiting for pre-heat start
	PreHeat, // pre-heating to starttemp
	NeedMalt, // Waiting for the malts to be added, manual proceed required
	Mashing, // Doing the mash steps
	Sparging, // keeps on endtemp temperature, and circulates
	PreBoil, // Heats the BK up to boiling, till the start of the boil timer
	Hopping, // BK being boild, hopping timers started
	Cooling, // The wort is being cooled down, later whirlpool and such
	Finished // Brewind process finished
    };
    typedef std::map<uint32_t, float> ThermoDataPoints;
    typedef std::function<void(States, States)> statechange_t;
  private:
    struct ThermoData {
      ThermoDataPoints readings;
      ThermoDataPoints moving5s;
      ThermoDataPoints derivate1st;
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
    ProcessState &setState(States _st);
    void registerStateChange(statechange_t _stch);
    inline uint32_t getStartat() const { return c_startat; };
    inline uint32_t getVolume() const { return c_volume; };
    ProcessState &addThermoReading(const std::string &_sensor, const uint32_t _time, const float _temp);
    ProcessState &getThermoCouples(std::set<std::string> &_tcs);
    ProcessState &getTCReadings(const std::string &_sensor, ThermoDataPoints &_tcvals);
    inline float getSensorTemp(const std::string &_sensor) const {return c_lasttemps.find(_sensor)->second;};

  protected:
    std::recursive_mutex c_mtx_state;
  private:
    // state change callbacks
    std::list<statechange_t> c_stcbs;
    // inputs
    std::shared_ptr<Program> c_program;
    uint32_t c_startat;
    uint32_t c_volume; //liters
    uint32_t c_startedat; //when we went from !isactive->isactive
    // cache the last readings
    std::map<std::string, float> c_lasttemps;
    // states
    std::atomic<States> c_state;
    std::map<std::string, ThermoData> c_thermoreadings;
  };
}

#endif
