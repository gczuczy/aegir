
#ifndef AEGIR_UNITS_H
#define AEGIR_UNITS_H

#include <math.h>

namespace aegir {
	// really this is the Plato formula, as that was consistent with all online
	// sg->brix conversion calculators
	// this brix formula is commented out due to this reason
	static constexpr float c0 = -613.9427;
	static constexpr float c1 = 1102.9079;
	static constexpr float c2 = -622.5576;
	static constexpr float c3 = 133.5892;

	inline float sg2brix(float _sg) {
		//return 182.4601*powf(_sg, 3) - 775.6821*powf(_sg,2) + 1262.7794*_sg - 669.5622;
		return c3*powf(_sg, 3) + c2*powf(_sg, 2) + c1*_sg + c0;
	}

	inline float brix2sg(float _brix) {
		return 1+(_brix/(258.6-((_brix/258.2)*227.1)));
	}
}

#endif
