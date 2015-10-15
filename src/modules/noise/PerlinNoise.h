#pragma once

#include "stb_perlin.h"

namespace noise {

class PerlinNoise {
public:
	double get(double x, double y, double z, double worldDimension);

	void setSeed(long seed) {
	}
};

}
