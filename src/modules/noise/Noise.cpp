/**
 * @file
 */

#include "Noise.h"
#include "core/Trace.h"
#include "Simplex.h"

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

}
