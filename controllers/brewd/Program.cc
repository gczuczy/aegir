
#include "Program.hh"

namespace aegir {
  Program::Program(uint32_t _progid, float _starttemp, float _endtemp, uint16_t _boiltime,
		   bool _nomash, bool _noboil,
		   const MashSteps &_ms, const Hops _hops):
    c_progid(_progid), c_starttemp(_starttemp), c_endtemp(_endtemp), c_boiltime(_boiltime),
    c_nomash(_nomash), c_noboil(_noboil),
    c_mashsteps(_ms), c_hops(_hops) {
  }

  Program::~Program() {
  }

}
