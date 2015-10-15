#pragma once
#include <Noise.h>

namespace noise {

class NoisePPNoise {
private:
	noisepp::Real *_buffer;
	int _w = 64;
	int _h = 64;

public:
	void init();

	double get(double x, double y, double z, double worldDimension);

	void setSeed(long seed) {
	}
};

}
