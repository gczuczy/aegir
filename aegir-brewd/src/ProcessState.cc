
#include "ProcessState.hh"
#include "Exception.hh"
#include "Config.hh"

#include <map>

namespace aegir {

  /*
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
   */
  static std::map<ProcessState::States, std::string> g_strstates{
    {ProcessState::States::Empty, "Empty"},
      {ProcessState::States::Loaded, "Loaded"},
      {ProcessState::States::PreWait, "PreWait"},
      {ProcessState::States::PreHeat, "PreHeat"},
      {ProcessState::States::Mashing, "Mashing"},
      {ProcessState::States::Sparging, "Sparging"},
      {ProcessState::States::PreBoil, "PreBoil"},
      {ProcessState::States::Hopping, "Hopping"},
      {ProcessState::States::Cooling, "Cooling"},
	{ProcessState::States::Finished, "Finished"}
  };

  ProcessState::ProcessState() {
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
    for ( auto &it: tcs ) {
      c_thermoreadings[it.first] = std::map<uint32_t, double>();
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
    c_volume = _volume;
    // clear the thermo readings
    for ( auto &it: c_thermoreadings ) it.second.clear();

    c_state = States::Loaded;
    return *this;
  }

  std::shared_ptr<Program> ProcessState::getProgram() {
    std::lock_guard<std::recursive_mutex> guard(c_mtx_state);
    return c_program;
  }

  std::string ProcessState::getStringState() const {
    return g_strstates[c_state];
  }

  ProcessState &ProcessState::addThermoReading(const std::string &_sensor, const uint32_t _time, const double _temp) {
    std::lock_guard<std::recursive_mutex> guard(c_mtx_state);
    auto it = c_thermoreadings.find(_sensor);

    // see whether we indeed have that TC
    if ( it == c_thermoreadings.end() )
      throw Exception("ProcessState::adThermoReading(): No such TC: %s", _sensor.c_str());

    // add the reading
    it->second[_time] = _temp;
    printf("ProcessState::addThemoReading(): added %s/%u/%.2f\n", _sensor.c_str(), _time, _temp);
    return *this;
  }

  ProcessState &ProcessState::getThermoCouples(std::set<std::string> &_tcs){
    _tcs.clear();
    std::lock_guard<std::recursive_mutex> guard(c_mtx_state);
    for ( auto &it: c_thermoreadings )
      _tcs.insert(it.first);
    return *this;
  }

  ProcessState &ProcessState::getTCReadings(const std::string &_sensor, std::map<uint32_t, double> &_tcvals){
    std::lock_guard<std::recursive_mutex> guard(c_mtx_state);

    auto it = c_thermoreadings.find(_sensor);
    if ( it == c_thermoreadings.end() )
      throw Exception("ProcessState::getTCReadings(): No such TC: %s", _sensor.c_str());

    _tcvals = it->second;

    return *this;
  }

}
