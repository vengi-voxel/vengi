/**
 * @file
 */

#pragma once

#include "Simplex.h"
#include <glm/trigonometric.hpp>
#include <glm/gtc/constants.hpp>
#include <cstdint>

namespace noise {

/**
 * @brief Normalizes a noise value in the range [-1,-1] to [0,1]
 */
inline float norm(float noise) {
	return (glm::clamp(noise, -1.0f, 1.0f) + 1.0f) * 0.5f;
}

/**
 * @return A value between [-amplitude*octaves*persistence,amplitude*octaves*persistence]
 * @param[in] octaves the amount of noise calls that contribute to the final result
 * @param[in] persistence the persistence defines how much of the amplitude will be applied to the next noise call (only makes
 * sense if you have @c octaves > 1). The higher this value is (ranges from 0-1) the more each new octave will add to the result.
 * @param[in] frequency the higher the @c frequency the more deviation you get in your noise (wavelength).
 * @param[in] amplitude the amplitude defines how high the noise will be.
 */
extern float Noise2D(const glm::vec2& pos, int octaves = 1, float persistence = 1.0f, float frequency = 1.0f, float amplitude = 1.0f);

/**
 * @return A value between [-amplitude*octaves*persistence,amplitude*octaves*persistence]
 * @param[in] octaves the amount of noise calls that contribute to the final result
 * @param[in] persistence the persistence defines how much of the amplitude will be applied to the next noise call (only makes
 * sense if you have @c octaves > 1). The higher this value is (ranges from 0-1) the more each new octave will add to the result.
 * @param[in] frequency the higher the @c frequency the more deviation you get in your noise (wavelength).
 * @param[in] amplitude the amplitude defines how high the noise will be.
 */
extern float Noise3D(const glm::vec3& pos, int octaves = 1, float persistence = 1.0f, float frequency = 1.0f, float amplitude = 1.0f);

/**
 * @return A value between [-amplitude*octaves*persistence,amplitude*octaves*persistence]
 * @param[in] octaves the amount of noise calls that contribute to the final result
 * @param[in] persistence the persistence defines how much of the amplitude will be applied to the next noise call (only makes
 * sense if you have @c octaves > 1). The higher this value is (ranges from 0-1) the more each new octave will add to the result.
 * @param[in] frequency the higher the @c frequency the more deviation you get in your noise (wavelength).
 * @param[in] amplitude the amplitude defines how high the noise will be.
 */
extern float Noise4D(const glm::vec4& pos, int octaves = 1, float persistence = 1.0f, float frequency = 1.0f, float amplitude = 1.0f);

/**
 * @brief Fills the given target buffer with RGB or RGBA values for the noise (depending on the components).
 * @param[in] buffer pointer to the target buffer - must be of size @c width * height * 3
 * @param[in] width the width of the image. Make sure that the target buffer has enough space to
 * store the needed data for the dimensions you specify here.
 * @param[in] height the height of the image. Make sure that the target buffer has enough space
 * to store the needed data for the dimensions you specify here.
 * @param[in] components 4 for RGBA and 3 for RGB
 * @param[in] octaves the amount of noise calls that contribute to the final result
 * @param[in] persistence the persistence defines how much of the amplitude will be applied to the next noise call (only makes
 * sense if you have @c octaves > 1). The higher this value is (ranges from 0-1) the more each new octave will add to the result.
 * @param[in] frequency the higher the @c frequency the more deviation you get in your noise (wavelength).
 * @param[in] amplitude the amplitude defines how high the noise will be.
 */
inline void SeamlessNoise2DRGB(uint8_t* buffer, int size, int octaves = 1, float persistence = 1.0f, float frequency = 1.0f, float amplitude = 1.0f) {
	const int components = 3;
	uint8_t bufferChannel[size * size];
	const float pi2 = glm::two_pi<float>();
	const float d = 1.0f / size;
	for (int channel = 0; channel < components; ++channel) {
		// seamless noise: http://www.gamedev.net/blog/33/entry-2138456-seamless-noise/
		float s = 0.0f;
		for (int x = 0; x < size; x++, s += d) {
			const float s_pi2 = s * pi2;
			const float nx = glm::cos(s_pi2);
			const float nz = glm::sin(s_pi2);
			float t = 0.0f;
			for (int y = 0; y < size; y++, t += d) {
				const float t_pi2 = t * pi2;
				const float ny = glm::cos(t_pi2);
				const float nw = glm::sin(t_pi2);
				float noise = noise::Noise4D(glm::vec4(nx, ny, nz, nw) + glm::vec4(channel), octaves, persistence, frequency, amplitude);
				noise = noise::norm(noise);
				const unsigned char color = (unsigned char) (noise * 255.0f);
				const int channelIndex = y * size + x;
				bufferChannel[channelIndex + 1] = color;
			}
		}
		int index = 0;
		for (int x = 0; x < size; ++x) {
			for (int y = 0; y < size; ++y, ++index) {
				buffer[index * components + channel] = bufferChannel[index];
			}
		}
	}
}

/**
 * @return Range [-+2147483647,+2147483647].
 */
extern int32_t intValueNoise(const glm::ivec3& pos, int32_t seed = 0);
/**
 * @return Range [-1.0,+1.0].
 */
extern double doubleValueNoise(const glm::ivec3& pos, int32_t seed = 0);

extern double voronoi(const glm::dvec3& pos, bool enableDistance, double frequency = 1.0, int seed = 0);

/**
 * @param lacunarity spacing between successive octaves (use exactly 2.0 for wrapping output)
 * @param gain relative weighting applied to each successive octave
 */
extern float swissTurbulence(const glm::vec2& p, float offset, int octaves, float lacunarity = 2.0f, float gain = 0.6f, float warp = 0.15f);
/**
 * @param lacunarity spacing between successive octaves (use exactly 2.0 for wrapping output)
 * @param gain relative weighting applied to each successive octave
 */
extern float jordanTurbulence(const glm::vec2&p, float offset, int octaves, float lacunarity = 2.0f, float gain1 = 0.8f, float gain = 0.5f, float warp0 = 0.4f, float warp = 0.35f,
		float damp0 = 1.0f, float damp = 0.8f, float damp_scale = 1.0f);

}
