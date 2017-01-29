/**
 * @file
 */

#include "SimplexNoise.h"
#include "core/Trace.h"

#define GLM_NOISE 1
#define FAST_NOISE 0

#if FAST_NOISE != 0
#include <FastNoise.h>
#endif

namespace noise {

/**
 * @param[in] octaves The amount of octaves controls the level of detail. Adding more octaves increases the detail level, but also the computation time.
 * @param[in] persistence A multiplier that defines how fast the amplitude diminishes for each successive octave.
 * @param[in] lacunarity A multiplier that defines how quickly the frequency changes for each successive octave.
 * @param[in] amplitude The maximum absolute value that the noise function can output.
 */
template<class VecType>
static float Noise(const VecType& pos, int octaves, float persistence, float lacunarity, float frequency, float amplitude) {
	core_trace_scoped(Noise);
#if FAST_NOISE
	static FastNoise fn;
	fn.SetNoiseType(FastNoise::NoiseType::SimplexFractal);
	fn.SetFractalOctaves(octaves);
	fn.SetFrequency(frequency);
	fn.SetFractalLacunarity(lacunarity);
	fn.SetFractalGain(persistence);
	fn.SetFractalType(FastNoise::FractalType::RigidMulti);
	return fn.GetSimplexFractal(pos);
#endif
#if GLM_NOISE
	float total = 0.0f;
	for (int i = 0; i < octaves; ++i) {
		total += glm::simplex(pos * frequency) * amplitude;
		frequency *= lacunarity;
		amplitude *= persistence;
	}
	return total;
#endif
}

/**
 * @param[in] octaves The amount of octaves controls the level of detail. Adding more octaves increases the detail level, but also the computation time.
 * @param[in] persistence A multiplier that defines how fast the amplitude diminishes for each successive octave.
 * @param[in] lacunarity A multiplier that defines how quickly the frequency changes for each successive octave.
 */
template<class VecType>
static float NoiseClamped(const VecType& pos, int octaves, float persistence, float lacunarity, float frequency) {
	core_trace_scoped(NoiseClamped);
	float total = 0.0f;
	float maxAmplitude = 0;
	float amplitude = 1.0f;
#if FAST_NOISE
	static FastNoise fn;
	fn.SetNoiseType(FastNoise::NoiseType::Simplex);
#endif
	for (int i = 0; i < octaves; ++i) {
#if GLM_NOISE
		total += glm::simplex(pos * frequency) * amplitude;
#elif FAST_NOISE
		fn.SetFrequency(frequency);
		total += fn.GetSimplex(pos) * amplitude;
#endif
		frequency *= lacunarity;
		maxAmplitude += amplitude;
		amplitude *= persistence;
	}
	return total / maxAmplitude;
}

float Simplex::Noise2D(const glm::vec2& pos, int octaves, float persistence, float frequency, float amplitude) {
	return Noise(pos, octaves, persistence, 2.0f, frequency, amplitude);
}

float Simplex::Noise3D(const glm::vec3& pos, int octaves, float persistence, float frequency, float amplitude) {
	return Noise(pos, octaves, persistence, 2.0f, frequency, amplitude);
}

float Simplex::Noise4D(const glm::vec4& pos, int octaves, float persistence, float frequency, float amplitude) {
	return Noise(pos, octaves, persistence, 2.0f, frequency, amplitude);
}

float Simplex::Noise2DClamped(const glm::vec2& pos, int octaves, float persistence, float frequency) {
	return NoiseClamped(pos, octaves, persistence, 2.0f, frequency);
}

float Simplex::Noise3DClamped(const glm::vec3& pos, int octaves, float persistence, float frequency) {
	return NoiseClamped(pos, octaves, persistence, 2.0f, frequency);
}

float Simplex::Noise4DClamped(const glm::vec4& pos, int octaves, float persistence, float frequency) {
	return NoiseClamped(pos, octaves, persistence, 2.0f, frequency);
}

}
