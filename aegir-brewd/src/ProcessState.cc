
#include "ProcessState.hh"
#include "Exception.hh"
#include "Config.hh"

#include <time.h>

#include <map>
#include <limits>

namespace aegir {

  static std::map<ProcessState::States, std::string> g_strstates{
    {ProcessState::States::Maintenance, "Maintenance"},
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
    {ProcessState::States::Transfer, "Transfer"},
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
      c_thermoreadings[it.first] = ThermoDataPoints();
      c_lasttemps[it.first] = 0;
    }

    reset();

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

    if ( c_state == States::Maintenance )
      throw Exception("Program loading is not allowed during maintenance");

    Guard g(*this);
    c_program = std::make_shared<Program>(_prog);
    c_startat = _startat;
    c_volume  = _volume;
    c_startedat = 0;
    // clear the thermo readings
    for ( auto &it: c_thermoreadings )  it.second.clear();

    setState(States::Loaded);
    return *this;
  }

  std::shared_ptr<Program> ProcessState::getProgram() {
    Guard g(*this);
    return c_program;
  }

  ProcessState &ProcessState::setState(ProcessState::States _st) {
    Guard g(*this);

#if 0
    printf("ProcessState::setState(%s): from %s\n", g_strstates[_st].c_str(),
	   g_strstates[c_state].c_str());
#endif
    // state can't decrease
    // except when resetting to Empty
    if ( (_st != States::Empty && _st != States::Maintenance) && _st <= c_state )
      throw Exception("ProcessState::setSate(%s): can't decrease the state", g_strstates[_st].c_str());

    // once we need to start keeping track of the time,
    // the reference time is noted
    if ( c_state < States::Mashing && _st >= States::Mashing )
      c_startedat = time(0);

    if ( c_state <= States::Sparging && _st > States::Sparging )
      c_t_endsparge = time(0);

    // and finally set the state
    States old(c_state);
    c_state = _st;
    for (auto &it: c_stcbs) {
      it(old, c_state);
    }
#ifdef AEGIR_DEBUG
    int sa = c_startedat;
    printf("State changed to %s startedat:%i\n", g_strstates[c_state].c_str(), sa);
#endif
    return *this;
  }

  ProcessState &ProcessState::reset() {
    Guard g(*this);

    c_program = nullptr;
    c_startat = 0;
    c_volume = 0;
    c_startedat = 0;
    c_t_endsparge = std::numeric_limits<uint32_t>::max();

    for (auto &it: c_lasttemps) it.second = 0;

    for (auto &it: c_thermoreadings) it.second.clear();

    c_mashstep = -1;
    c_mashstepstart = 0;
    c_targettemp = 0;
    c_maint_heat = false;
    c_maint_pump = false;
    c_maint_bkpump = false;
    c_levelerror = false;
    c_maint_temp = 37;
    c_t_hopstart = 0;
    c_hopid = 0;

    c_force_mtpump = false;
    c_block_heat = false;

    setState(States::Empty);

    return *this;
  }

  std::string ProcessState::getStringState() const {
    return g_strstates[c_state];
  }

  ProcessState::States ProcessState::byString(const std::string &_state) const {
    States st;

    for ( auto const& [s, str]: g_strstates) {
      if ( _state == str ) return s;
    }

    throw Exception("Unknown state");
  }

  void ProcessState::registerStateChange(statechange_t _stch) {
    Guard g(*this);
    c_stcbs.push_back(_stch);
  }

  ProcessState &ProcessState::addThermoReadings(const time_t _time,
						const ThermoReadings _temps) {
    if ( c_state == States::Empty ||
	 c_state == States::Finished ) return *this;
    Guard g(*this);

    // add the reading
    if ( c_state >= States::Mashing )
      c_thermoreadings.insert(_time, _temps);
    return *this;
  }
}
