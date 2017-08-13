
#include "ProcessState.hh"
#include "Exception.hh"
#include "Config.hh"

#include <time.h>

#include <map>

namespace aegir {

  static std::map<ProcessState::States, std::string> g_strstates{
    {ProcessState::States::Empty, "Empty"},
      {ProcessState::States::Loaded, "Loaded"},
      {ProcessState::States::PreWait, "PreWait"},
      {ProcessState::States::PreHeat, "PreHeat"},
      {ProcessState::States::NeedMalt, "NeedMalt"},
      {ProcessState::States::Mashing, "Mashing"},
      {ProcessState::States::Sparging, "Sparging"},
      {ProcessState::States::PreBoil, "PreBoil"},
      {ProcessState::States::Hopping, "Hopping"},
      {ProcessState::States::Cooling, "Cooling"},
	{ProcessState::States::Finished, "Finished"}
  };

  ProcessState::Guard::Guard(ProcessState &_ps): c_ps(_ps) {
    c_ps.c_mtx_state.lock();
  }

  ProcessState::Guard::~Guard() {
    c_ps.c_mtx_state.unlock();
  }

  ProcessState::ProcessState(): c_state(States::Empty), c_startedat(0) {
    // Initialize the internals
    reconfigure();
  }

  ProcessState::~ProcessState() {
  }

  ProcessState &ProcessState::getInstance() {
    static ProcessState ps;
    return ps;
  }

  ProcessState &ProcessState::reconfigure() {
    Config *cfg = Config::getInstance();

    std::map<std::string, int> tcs;
    cfg->getThermocouples(tcs);
    c_thermoreadings.clear();
    c_lasttemps.clear();
    for ( auto &it: tcs ) {
      c_thermoreadings[it.first] = ThermoData();
      c_lasttemps[it.first] = 0;
    }
    return *this;
  }

  bool ProcessState::isActive() const {
    States state = c_state;
    if ( state == States::Empty ||
	 state == States::Loaded ||
	 state == States::Finished )
      return false;
    return true;
  }

  ProcessState &ProcessState::loadProgram(const Program &_prog, uint32_t _startat, uint32_t _volume) {
    if ( isActive() )
      throw Exception("Cannot load program: brew process active");

    std::lock_guard<std::recursive_mutex> guard(c_mtx_state);
    c_program = std::make_shared<Program>(_prog);
    c_startat = _startat;
    c_volume  = _volume;
    c_startedat = 0;
    // clear the thermo readings
    for ( auto &it: c_thermoreadings ) {
      it.second.readings.clear();
      it.second.moving5s.clear();
      it.second.derivate1st.clear();
    }

    setState(States::Loaded);
    return *this;
  }

  std::shared_ptr<Program> ProcessState::getProgram() {
    std::lock_guard<std::recursive_mutex> guard(c_mtx_state);
    return c_program;
  }

  ProcessState &ProcessState::setState(ProcessState::States _st) {
    std::lock_guard<std::recursive_mutex> guard(c_mtx_state);

    // statecan't decrease
    if ( _st <= c_state )
      throw Exception("ProcessState::setSate(%s): can't decrease the state", g_strstates[_st].c_str());

    // once we need to start keeping track of the time,
    // the reference time is noted
    if ( c_state < States::Mashing && _st >= States::Mashing )
      c_startedat = time(0);

    // and finally set the state
    States old(c_state);
    c_state = _st;
    for (auto &it: c_stcbs) {
      it(old, c_state);
    }
#ifdef AEGIR_DEBUG
    printf("State changed to %s\n", g_strstates[c_state].c_str());
#endif
    return *this;
  }

  std::string ProcessState::getStringState() const {
    return g_strstates[c_state];
  }

  void ProcessState::registerStateChange(statechange_t _stch) {
    std::lock_guard<std::recursive_mutex> guard(c_mtx_state);
    c_stcbs.push_back(_stch);
  }

  ProcessState &ProcessState::addThermoReading(const std::string &_sensor, const uint32_t _time, const float _temp) {
    if ( c_state == States::Empty ||
	 c_state == States::Finished ) return *this;
    std::lock_guard<std::recursive_mutex> guard(c_mtx_state);
    auto it = c_thermoreadings.find(_sensor);

    // see whether we indeed have that TC
    if ( it == c_thermoreadings.end() )
      throw Exception("ProcessState::adThermoReading(): No such TC: %s", _sensor.c_str());

    // if a program is loaded, then save the temperature as the current
    if ( c_state >= States::Loaded )
      c_lasttemps[_sensor] = _temp;

    // add the reading
    if ( c_state >= States::Mashing ) {
      it->second.readings[_time - c_startedat] = _temp;
      printf("ProcessState::addThemoReading(): added %s/%u/%.2f\n", _sensor.c_str(), _time, _temp);
    }
    return *this;
  }

  ProcessState &ProcessState::getThermoCouples(std::set<std::string> &_tcs){
    _tcs.clear();
    std::lock_guard<std::recursive_mutex> guard(c_mtx_state);
    for ( auto &it: c_thermoreadings )
      _tcs.insert(it.first);
    return *this;
  }

  ProcessState &ProcessState::getTCReadings(const std::string &_sensor, ThermoDataPoints &_tcvals){
    std::lock_guard<std::recursive_mutex> guard(c_mtx_state);

    auto it = c_thermoreadings.find(_sensor);
    if ( it == c_thermoreadings.end() )
      throw Exception("ProcessState::getTCReadings(): No such TC: %s", _sensor.c_str());

    _tcvals = it->second.readings;

    return *this;
  }

}
