
#include "Program.hh"

namespace aegir {
  Program::Program(uint32_t _progid, float _starttemp, float _endtemp,
		   uint16_t _boiltime, const MashSteps &_ms, const Hops _hops):
    c_progid(_progid), c_starttemp(_starttemp), c_endtemp(_endtemp),
    c_boiltime(_boiltime), c_mashsteps(_ms), c_hops(_hops) {
  }

  Program::~Program() {
  }

}
