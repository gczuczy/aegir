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
#include <ctime>

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
      Maintenance=0, // Maintenance mode
      Empty, // initialized, no program loadad
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
    ProcessState &reset();
    void registerStateChange(statechange_t _stch);
    inline uint32_t getStartat() const { return c_startat; };
    inline uint32_t getVolume() const { return c_volume; };
    inline ProcessState &setVolume(uint32_t _v) { c_volume = _v; return *this; };
    ProcessState &addThermoReading(const std::string &_sensor, const uint32_t _time, const float _temp);
    ProcessState &getThermoCouples(std::set<std::string> &_tcs);
    ProcessState &getTCReadings(const std::string &_sensor, ThermoDataPoints &_tcvals);
    inline float getSensorTemp(const std::string &_sensor) const {return c_lasttemps.find(_sensor)->second;};
    //inline time_t getStartedAt() const {return c_startedat;};
    uint32_t getStartedAt() const;
    uint32_t getEndSparge() const {uint32_t x=c_t_endsparge; return x;};
    // mash steps
    inline ProcessState &setMashStep(int8_t _ms) {c_mashstep=_ms; return *this;};
    inline int8_t getMashStep() const {return c_mashstep;};
    inline ProcessState &setMashStepStart(time_t _mst) {c_mashstepstart = _mst; return *this;};
    inline time_t getMashStepStart() const {return c_mashstepstart;};
    // target temperatures
    inline ProcessState &setTargetTemp(float _tt) { c_targettemp = _tt; return *this; };
    inline float getTargetTemp() const { return c_targettemp;};
    // maintenance mode
    inline ProcessState &setMaintPump(bool _val) {c_maint_pump = _val; return *this; };
    inline bool getMaintPump() {return c_maint_pump; };
    inline ProcessState &setMaintHeat(bool _val) {c_maint_heat = _val; return *this; };
    inline bool getMaintHeat() {return c_maint_heat; };
    inline ProcessState &setMaintTemp(float _val) {c_maint_temp = _val; return *this; };
    inline float getMaintTemp() {return c_maint_temp; };
    inline ProcessState &setHoppingStart(uint32_t _val) {c_t_hopstart = _val; return *this; };
    inline uint32_t getHoppingStart() { return c_t_hopstart; };
    inline ProcessState &setHopId(uint32_t _val) { c_hopid = _val; return *this; };
    inline uint32_t getHopId() { return c_hopid; };
    inline ProcessState &setHopTime(uint32_t _val) { c_t_hoptime = _val; return *this; };
    inline uint32_t getHopTime() const { return c_t_hoptime; };
    inline ProcessState &setForcePump(bool _val) { c_force_pump = _val; return *this; };
    inline bool getForcePump() { return c_force_pump; };
    inline ProcessState &setBlockHeat(bool _val) { c_block_heat = _val; return *this; };
    inline bool getBlockHeat() { return c_block_heat; };

  protected:
    std::recursive_mutex c_mtx_state;
  private:
    // state change callbacks
    std::list<statechange_t> c_stcbs;
    // inputs
    std::shared_ptr<Program> c_program;
    uint32_t c_startat;
    std::atomic<uint32_t> c_volume; //liters
    std::atomic<uint32_t> c_startedat; //when we went from !isactive->isactive
    std::atomic<uint32_t> c_t_endsparge; // when sparging is done
    // cache the last readings
    std::map<std::string, float> c_lasttemps;
    // states
    std::atomic<States> c_state;
    std::map<std::string, ThermoDataPoints> c_thermoreadings;
    // active mash step
    std::atomic<int8_t> c_mashstep;
    std::atomic<time_t> c_mashstepstart;
    // the current target temp
    std::atomic<float> c_targettemp;
    // start of hopping
    std::atomic<uint32_t> c_t_hopstart;
    std::atomic<uint32_t> c_hopid; // for the UI
    std::atomic<uint32_t> c_t_hoptime; // for the UI
    // maintmode variables
    std::atomic<bool> c_maint_pump;
    std::atomic<bool> c_maint_heat;
    std::atomic<float> c_maint_temp;
    // sparge/boil/cool forcings
    std::atomic<bool> c_force_pump;
    std::atomic<bool> c_block_heat;
  };
}

#endif
