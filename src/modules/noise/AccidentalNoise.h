#pragma once

#include "core/Random.h"
#include "core/Common.h"
#include <anl_noise.h>
#include <anl_rgba.h>
#include <anl_imaging.h>
#include <mapping.h>

namespace noise {

class AccidentalNoise {
private:
#if 0
	anl::CImplicitConstant air;
	anl::CImplicitConstant plain;
	anl::CImplicitConstant highland;
	anl::CImplicitConstant mountain;

	anl::CImplicitGradient groundGradient;

	anl::CImplicitFractal plainFractal;
	anl::CImplicitAutoCorrect plainAutoCorrect;
	anl::CImplicitScaleOffset plainScaleOffset;
	anl::CImplicitScaleDomain plainScaleDomain;
	anl::CImplicitTranslateDomain plainTranslateDomain;

	anl::CImplicitFractal highlandFractal;
	anl::CImplicitAutoCorrect highlandAutoCorrect;
	anl::CImplicitScaleOffset highlandScaleOffset;
	anl::CImplicitScaleDomain highlandScaleDomain;
	anl::CImplicitTranslateDomain highlandTranslateDomain;

	anl::CImplicitFractal mountainFractal;
	anl::CImplicitAutoCorrect mountainAutoCorrect;
	anl::CImplicitScaleOffset mountainScaleOffset;
	anl::CImplicitScaleDomain mountainScaleDomain;
	anl::CImplicitTranslateDomain mountainTranslateDomain;

	anl::CImplicitFractal terrainControl;
	anl::CImplicitAutoCorrect terrainAutoCorrect;
	anl::CImplicitScaleOffset terrainScaleOffset;
	anl::CImplicitScaleDomain terrainScaleDomain;
	anl::CImplicitCache terrainCache;

	anl::CImplicitSelect highlandMountainSelect;
	anl::CImplicitSelect highlandMountainColorSelect;
	anl::CImplicitCache highlandMountainColorCache;
	anl::CImplicitSelect plainHighlandSelect;
	anl::CImplicitSelect plainHighlandColorSelect;
	anl::CImplicitCache plainHighlandCache;

	anl::CImplicitSelect groundBase;
#endif
	anl::CImplicitFractal plainFractal;

public:
	AccidentalNoise();

	double get(double x, double y, double z, double worldDimension);

	void setSeed(long seed);
};

inline double AccidentalNoise::get(double x, double y, double z, double worldDimension) {
	const double scale = 1.0 / worldDimension;
	return plainFractal.get(x * scale, y * scale, z * scale);
}

}
