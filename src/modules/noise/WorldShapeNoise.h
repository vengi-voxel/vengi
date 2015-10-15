#pragma once

#include "core/Random.h"
#include "core/Common.h"
#include <anl_noise.h>
#include <anl_rgba.h>
#include <anl_imaging.h>
#include <mapping.h>

namespace noise {

class WorldShapeNoise {
private:
	anl::CImplicitSphere _sphere;
	anl::CImplicitMath _shape;

	anl::CImplicitFractal _elevationBase;
	anl::CImplicitAutoCorrect _elevationAutoCorrect;

	anl::CImplicitFractal _elevationTurbulence;
	anl::CImplicitAutoCorrect _elevationTurbulenceAutoCorrect;

	anl::CImplicitTranslateDomain _elevationTurbulenceTranslateDomain;
	anl::CImplicitMath _elevation;

	anl::CImplicitConstant _air;
	anl::CImplicitConstant _soil;

	anl::CImplicitSelect _groundBase;

public:
	WorldShapeNoise();

	double get(double x, double y, double z, double worldDimension = 128.0);

	void generateImage();

	void setSeed(long seed);
};

inline double WorldShapeNoise::get(double x, double y, double z, double worldDimension) {
	const double scale = 1.0 / worldDimension;
	return _groundBase.get(x * scale, y * scale, z * scale);
}

}
