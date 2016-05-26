/**
 * @file
 */

#include "SimplexNoise.h"
#include "core/Trace.h"

namespace noise {

template<class VecType>
static float Noise(const VecType& pos, int octaves, float persistence, float frequency, float amplitude) {
	core_trace_scoped(Noise);
	float total = 0.0f;
	for (int i = 0; i < octaves; ++i) {
		total += glm::simplex(pos * frequency) * amplitude;
		frequency *= 2.0f;
		amplitude *= persistence;
	}
	return total;
}

template<class VecType>
static float NoiseClamped(const VecType& pos, int octaves, float persistence, float frequency) {
	core_trace_scoped(Noise);
	float total = 0.0f;
	float maxAmplitude = 0;
	float amplitude = 1.0f;
	for (int i = 0; i < octaves; ++i) {
		total += glm::simplex(pos * frequency) * amplitude;
		frequency *= 2.0f;
		maxAmplitude += amplitude;
		amplitude *= persistence;
	}
	return total / maxAmplitude;
}

float Simplex::Noise2D(const glm::vec2& pos, int octaves, float persistence, float frequency, float amplitude) {
	return Noise(pos, octaves, persistence, frequency, amplitude);
}

float Simplex::Noise3D(const glm::vec3& pos, int octaves, float persistence, float frequency, float amplitude) {
	return Noise(pos, octaves, persistence, frequency, amplitude);
}

float Simplex::Noise4D(const glm::vec4& pos, int octaves, float persistence, float frequency, float amplitude) {
	return Noise(pos, octaves, persistence, frequency, amplitude);
}

float Simplex::Noise2DClamped(const glm::vec2& pos, int octaves, float persistence, float frequency) {
	return NoiseClamped(pos, octaves, persistence, frequency);
}

float Simplex::Noise3DClamped(const glm::vec3& pos, int octaves, float persistence, float frequency) {
	return NoiseClamped(pos, octaves, persistence, frequency);
}

float Simplex::Noise4DClamped(const glm::vec4& pos, int octaves, float persistence, float frequency) {
	return NoiseClamped(pos, octaves, persistence, frequency);
}

}
