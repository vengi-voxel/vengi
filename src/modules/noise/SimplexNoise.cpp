#include "SimplexNoise.h"
#include <glm/gtc/noise.hpp>

namespace noise {

template<class VecType>
static float Noise(const VecType& pos, int octaves, float persistence, float frequency, float amplitude) {
	float total = 0.0f;
	for (int i = 0; i < octaves; ++i) {
		total += glm::simplex(pos * frequency) * amplitude;
		frequency *= 2.0f;
		amplitude *= persistence;
	}
	return total;
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

}
