/**
 * @file
 */

#include "Noise.h"
#include "core/Trace.h"
#include "Simplex.h"
#include <limits>

#define GLM_NOISE 0
#define CINDER_NOISE 1

namespace noise {

/**
 * @brief Fractional Brownian Motion
 *
 * fBM (fractional Brownian motion) is a composite Perlin noise algorithm. It creates more turbolence with more octaves.
 *
 * To cover all possible scales, the octaves are typically a bit less than @code log(width) / log(lacunarity)@endcode.
 * So, for a 1024x1024 heightfield, about 10 octaves are needed. The persistence influences the terrain turbolence.
 *
 * @param[in] octaves The amount of octaves controls the level of detail. Adding more octaves increases the detail level, but also the computation time.
 * @param[in] persistence A multiplier that defines how fast the amplitude diminishes for each successive octave.
 * @param[in] lacunarity A multiplier that defines how quickly the frequency changes for each successive octave.
 * @param[in] amplitude The maximum absolute value that the noise function can output.
 */
template<class VecType>
static float Noise(const VecType& pos, int octaves, float persistence, float lacunarity, float frequency, float amplitude) {
	core_trace_scoped(Noise);
	float total = 0.0f;
	for (int i = 0; i < octaves; ++i) {
#if GLM_NOISE == 1
		total += glm::simplex(pos * frequency) * amplitude;
#elif CINDER_NOISE == 1
		total += noise(pos * frequency) * amplitude;
#endif
		frequency *= lacunarity;
		amplitude *= persistence;
	}
	return total;
}

float Noise2D(const glm::vec2& pos, int octaves, float persistence, float frequency, float amplitude) {
	return Noise(pos, octaves, persistence, 2.0f, frequency, amplitude);
}

float Noise3D(const glm::vec3& pos, int octaves, float persistence, float frequency, float amplitude) {
	return Noise(pos, octaves, persistence, 2.0f, frequency, amplitude);
}

float Noise4D(const glm::vec4& pos, int octaves, float persistence, float frequency, float amplitude) {
	return Noise(pos, octaves, persistence, 2.0f, frequency, amplitude);
}

int intValueNoise(const glm::ivec3& pos, int seed) {
	const int xgen = 1619;
	const int ygen = 31337;
	const int zgen = 6971;
	const int fixedseed = 1013;
	int n = (xgen * pos.x + ygen * pos.y + zgen * pos.z + fixedseed * seed) & std::numeric_limits<int32_t>::max();
	n = (n >> 13) ^ n;
	return (n * (n * n * 60493 + 19990303) + 1376312589) & std::numeric_limits<int32_t>::max();
}

double doubleValueNoise(const glm::ivec3& pos, int seed) {
	return 1.0 - ((double)intValueNoise(pos, seed) / 1073741824.0);
}

double voronoi(const glm::dvec3& pos, bool enableDistance, double displacement, double frequency, int seed) {
	const glm::dvec3 p = pos * frequency;
	const glm::ivec3 rp(
			(p.x >= 0.0 ? (int)(p.x + 0.5) : (int)(p.x - 0.5)),
			(p.y >= 0.0 ? (int)(p.y + 0.5) : (int)(p.y - 0.5)),
			(p.z >= 0.0 ? (int)(p.z + 0.5) : (int)(p.z - 0.5)));

	double minDist = (double)std::numeric_limits<int32_t>::max();
	glm::dvec3 vp(0.0);

	const int d = 2;
	for (int z = rp.z - d; z <= rp.z + d; ++z) {
		for (int y = rp.y - d; y <= rp.y + d; ++y) {
			for (int x = rp.x - d; x <= rp.x + d; ++x) {
				const glm::ivec3 c(x, y, z);
				const double noiseX = x + doubleValueNoise(c, seed);
				const double noiseY = y + doubleValueNoise(c, seed + 1);
				const double noiseZ = z + doubleValueNoise(c, seed + 2);
				const glm::dvec3 noisePos(noiseX, noiseY, noiseZ);
				//const double dist = glm::abs(noiseX) + glm::abs(noiseY) + glm::abs(noiseZ) + glm::length2(noisePos);
				const double dist = glm::length2(noisePos - p);
				if (dist < minDist) {
					minDist = dist;
					vp = noisePos;
				}
			}
		}
	}

	const double value = enableDistance ? glm::length(vp - p) * glm::root_three<double>() - 1.0 : 0.0;
	const double ret = doubleValueNoise(glm::ivec3(glm::floor(vp)));
	return ret;
}

}
