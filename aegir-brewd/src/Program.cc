
#include "Program.hh"

namespace aegir {
  Program::Program(float _starttemp, float _endtemp, uint16_t _boiltime, int _startat, const MashSteps &_ms, const Hops _hops):
    c_starttemp(_starttemp), c_endtemp(_endtemp), c_boiltime(_boiltime), c_startat(_startat), c_mashsteps(_ms), c_hops(_hops) {
  }

  Program::~Program() {
  }
}
