#include "WorldShapeNoise.h"

namespace noise {

WorldShapeNoise::WorldShapeNoise() :
	_sphere(1.0, 0.5, 0.0, 0.5),
	_shape(),

	_elevationBase(anl::EFractalTypes::FBM, anl::GRADIENT, anl::QUINTIC, 6, 2, true),
	_elevationAutoCorrect(&_elevationBase, 0.0, 1.0),

	_elevationTurbulence(anl::EFractalTypes::RIDGEDMULTI, anl::GRADIENT, anl::QUINTIC, 8, 1, true),
	_elevationTurbulenceAutoCorrect(&_elevationTurbulence, 0.0, 1.0),

	_elevationTurbulenceTranslateDomain(&_elevationAutoCorrect, 0.0, &_elevationTurbulenceAutoCorrect, 0.0),
	_elevation(anl::MULTIPLY, &_elevationTurbulenceTranslateDomain, &_shape),

	_groundBase()
{
	_shape.setOperation(anl::EASEQUINTIC);
	_shape.setSource(&_sphere);

	_air.setConstant(0.0);
	_soil.setConstant(1.0);

	_groundBase.setLowSource(&_soil);
	_groundBase.setHighSource(&_air);
	_groundBase.setControlSource(&_elevation);
	_groundBase.setThreshold(0.5);
}

void WorldShapeNoise::generateImage() {
	anl::CRGBACompositeChannels composite;

	composite.setMode(anl::RGB);
	composite.setRedSource(&_shape);
	composite.setGreenSource(&_shape);
	composite.setBlueSource(&_shape);
	composite.setAlphaSource(1.0);

	anl::TArray2D<TVec4D<float>> img(256, 256);

	anl::SMappingRanges ranges;
	ranges.mapx0 = -1;
	ranges.mapy0 = -1;
	ranges.mapx1 = 1;
	ranges.mapy1 = 1;
	mapRGBA2D(anl::SEAMLESS_NONE, img ,composite ,ranges, 0);

	//Just for debugging purpose
	saveRGBAArray((char*)"heightmap.tga", &img);
	Log::info("+++++++++++++++generateImage");
}

void WorldShapeNoise::setSeed(long seed) {
	anl::CMWC4096 rnd;
	rnd.setSeed(seed);
}
}
