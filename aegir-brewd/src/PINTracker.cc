
#include "PINTracker.hh"
#include "Exception.hh"
#include "Config.hh"
#include "ProcessState.hh"

namespace aegir {
  /*
   * PINTracker::PIN
   */
  PINTracker::PIN::PIN(const std::string &_name): c_name(_name) {
    c_value = false;
    c_newvalue = false;
  }

  PINTracker::PIN::~PIN() {
  }

  bool PINTracker::PIN::getValue() {
    return c_value;
  }

  void PINTracker::PIN::pushback() {
    c_value = c_newvalue;
  }

  /*
   * PINTracker::OutPIN
   */
  PINTracker::OutPIN::OutPIN(const std::string &_name, PINChanges &_pcq): PINTracker::PIN(_name), c_pcq(_pcq) {
    c_type = PINTracker::PIN::PINType::OUT;
  }

  PINTracker::OutPIN::~OutPIN() {
  }

  bool PINTracker::OutPIN::getValue() {
    return c_value;
  }

  /*
   * if newval!=oldval, then has to be in the queue
   * if newval==oldval, then hos to be absent from the queue
   */
  void PINTracker::OutPIN::setValue(bool _v) {
    c_newvalue = _v;

    auto it = c_pcq.find(this);
    if ( c_value != c_newvalue ) {
      if ( it == c_pcq.end() ) {
	c_pcq.insert(this);
      }
      return;
    }

    // chg == false here
    if ( it != c_pcq.end() ) {
      c_pcq.erase(it);
    }
  }

  /*
   * PINTracker::InPIN
   */
  PINTracker::InPIN::InPIN(const std::string &_name, PINChanges &_inch): PINTracker::PIN(_name), c_inch(_inch) {
    c_type = PINTracker::PIN::PINType::IN;
  }

  PINTracker::InPIN::~InPIN() {
  }

  bool PINTracker::InPIN::getValue() {
    return c_newvalue;
  }

  /*
   * if newval!=oldval, then has to be in the queue
   * if newval==oldval, then hos to be absent from the queue
   */
  void PINTracker::InPIN::setValue(bool _v) {
    c_newvalue = _v;

#ifdef AEGIR_DEBUG
    printf("PINTracker::InPIN::setValue(%s, %i->%i)\n", c_name.c_str(),
	   c_value?1:0, c_newvalue?1:0);
#endif

    auto it = c_inch.find(this);
    if ( c_value == c_newvalue ) {
      if ( it != c_inch.end() ) c_inch.erase(it);
      return;
    }
    if ( it == c_inch.end() )
      c_inch.insert(this);
  }

  /*
   * PINTracker
   */
  PINTracker::PINTracker() {
  }

  PINTracker::~PINTracker() {
    c_pinchangequeue.clear();
    c_inpinchanges.clear();
    c_pins.clear();
  }

  void PINTracker::reconfigure() {
    if ( ProcessState::getInstance().isActive() )
      throw Exception("PINTracker cannot be reconfigured while a process is in progress");

    // configure the pins
#if 0
    printf("PINTracker::reconfigure(): Wiping already configured pins:\n");
    for ( auto &it: c_pins )
      printf(" - PIN: %s\n", it.first.c_str());
#endif
    c_pins.clear();
    for ( auto &it: g_pinconfig ) {
      if ( it.second.mode == PinMode::IN ) {
	auto x = std::make_shared<InPIN>(it.first, c_inpinchanges);
	c_pins[it.first] = std::dynamic_pointer_cast<PIN>(x);
      } else if ( it.second.mode == PinMode::OUT ) {
	auto x = std::make_shared<OutPIN>(it.first, c_pinchangequeue);
	c_pins[it.first] = std::dynamic_pointer_cast<PIN>(x);
      }
    }
  }

  void PINTracker::startCycle() {
  }

  void PINTracker::endCycle() {
    if ( c_pinchangequeue.size() || c_inpinchanges.size() )
    for ( auto &it: c_inpinchanges ) it->pushback();
    c_inpinchanges.clear();
    for ( auto &it: c_pinchangequeue ) {
      handleOutPIN(*it);
      it->pushback();
    }
    c_pinchangequeue.clear();
  }

  std::shared_ptr<PINTracker::PIN> PINTracker::getPIN(const std::string &_name) {
    auto it = c_pins.find(_name);
    if ( it == c_pins.end() )
      throw Exception("Unknown PIN: %s", _name.c_str());

    return it->second;
  }

  void PINTracker::setPIN(const std::string &_name, bool _value) {
    auto it = c_pins.find(_name);
    if ( it != c_pins.end() ) {
      if ( it->second->getType() == PIN::PINType::OUT ) {
	auto sppin = std::static_pointer_cast<OutPIN>(it->second);
	sppin->setValue(_value);
	//it->second->setValue(_value);
      } else if ( it->second->getType() == PIN::PINType::IN ) {
	auto sppin = std::static_pointer_cast<InPIN>(it->second);
	sppin->setValue(_value);
      }
    }
  }

  bool PINTracker::hasChanged(const std::string &_name) {
    auto it = c_pins.find(_name);
    if ( it == c_pins.end() )
      throw Exception("PINTracker::hasChanged(%s): no such PIN", _name.c_str());
    return it->second->isChanged();
  }

  bool PINTracker::hasChanged(const std::string &_name, std::shared_ptr<PIN> &_pin) {
    auto it = c_pins.find(_name);
    if ( it == c_pins.end() )
      throw Exception("PINTracker::hasChanged(%s): no such PIN", _name.c_str());
    _pin = it->second;
    return _pin->isChanged();
  }
}
