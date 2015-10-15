#include "PerlinNoise.h"

#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"

namespace noise {

double PerlinNoise::get(double x, double y, double z, double worldDimension) {
	const double scale = 1.0 / worldDimension;
	const float height = stb_perlin_noise3(x * scale, y * scale, z * scale, 0, 0, 0);
	if (::fabs(height) >= y * scale)
		return 1.0;
	return 0.0;
}

}
