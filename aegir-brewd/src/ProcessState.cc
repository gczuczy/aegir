
#include "ProcessState.hh"
#include "Exception.hh"

namespace aegir {

  ProcessState::ProcessState() {
  }

  ProcessState::~ProcessState() {
  }

  ProcessState &ProcessState::getInstance() {
    static ProcessState ps;
    return ps;
  }

  bool ProcessState::isActive() {
    std::lock_guard<std::recursive_mutex> guard(c_mtx_state);
    return !!c_program;
  }

  ProcessState &ProcessState::loadProgram(const Program &_prog, uint32_t _startat, uint32_t _volume) {
    if ( isActive() )
      throw Exception("Cannot load program: brew process active");

    std::lock_guard<std::recursive_mutex> guard(c_mtx_state);
    c_program = std::make_shared<Program>(_prog);
    c_startat = _startat;
    c_volume = _volume;
    return *this;
  }

  std::shared_ptr<Program> ProcessState::getProgram() {
    std::lock_guard<std::recursive_mutex> guard(c_mtx_state);
    return c_program;
  }
}
