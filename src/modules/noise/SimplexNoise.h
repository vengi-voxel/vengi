/**
 * @file
 */

#pragma once

#include "core/GLM.h"
#include <cstdint>
#include <functional>

namespace noise {

/**
 * @brief Normalizes a noise value in the range [-1,-1] to [0,1]
 */
inline float norm(float noise) {
	return (glm::clamp(noise, -1.0f, 1.0f) + 1.0f) * 0.5f;
}

class Simplex {
public:
	/**
	 * @return A value between [-1,1]
	 * @param[in] octaves the amount of noise calls that contribute to the final result
	 * @param[in] persistence the persistence defines how much of the amplitude will be applied to the next noise call (only makes
	 * sense if you have @c octaves > 1). The higher this value is (ranges from 0-1) the more each new octave will add to the result.
	 * @param[in] frequency the higher the @c frequency the more deviation you get in your noise (wavelength).
	 * @param[in] amplitude the amplitude defines how high the noise will be.
	 */
	static float Noise2DClamped(const glm::vec2& pos, int octaves = 1, float persistence = 1.0f, float frequency = 1.0f, float amplitude = 1.0f);

	static float Noise2D(const glm::vec2& pos, int octaves = 1, float persistence = 1.0f, float frequency = 1.0f, float amplitude = 1.0f);

	/**
	 * @return A value between [-1,1]
	 * @param[in] octaves the amount of noise calls that contribute to the final result
	 * @param[in] persistence the persistence defines how much of the amplitude will be applied to the next noise call (only makes
	 * sense if you have @c octaves > 1). The higher this value is (ranges from 0-1) the more each new octave will add to the result.
	 * @param[in] frequency the higher the @c frequency the more deviation you get in your noise (wavelength).
	 * @param[in] amplitude the amplitude defines how high the noise will be.
	 */
	static float Noise3DClamped(const glm::vec3& pos, int octaves = 1, float persistence = 1.0f, float frequency = 1.0f, float amplitude = 1.0f);

	static float Noise3D(const glm::vec3& pos, int octaves = 1, float persistence = 1.0f, float frequency = 1.0f, float amplitude = 1.0f);

	/**
	 * @return A value between [-1,1]
	 * @param[in] octaves the amount of noise calls that contribute to the final result
	 * @param[in] persistence the persistence defines how much of the amplitude will be applied to the next noise call (only makes
	 * sense if you have @c octaves > 1). The higher this value is (ranges from 0-1) the more each new octave will add to the result.
	 * @param[in] frequency the higher the @c frequency the more deviation you get in your noise (wavelength).
	 * @param[in] amplitude the amplitude defines how high the noise will be.
	 */
	static float Noise4DClamped(const glm::vec4& pos, int octaves = 1, float persistence = 1.0f, float frequency = 1.0f, float amplitude = 1.0f);

	static float Noise4D(const glm::vec4& pos, int octaves = 1, float persistence = 1.0f, float frequency = 1.0f, float amplitude = 1.0f);

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
	static void Noise2DBuffer(uint8_t* buffer, int width, int height, int components, int octaves = 1, float persistence = 1.0f, float frequency = 1.0f, float amplitude = 1.0f) {
		Noise2DBuffer(buffer, width, height, components, glm::vec2(0), octaves, persistence, frequency, amplitude);
	}

	static void Noise2DBuffer(uint8_t* buffer, int width, int height, int components, const glm::vec2& pos, int octaves = 1, float persistence = 1.0f, float frequency = 1.0f, float amplitude = 1.0f) {
		Noise2DBuffer(buffer, width, height, components, pos, noise::Simplex::Noise2D, octaves, persistence, frequency, amplitude);
	}

	static void Noise2DBuffer(uint8_t* buffer, int width, int height, int octaves = 1, float persistence = 1.0f, float frequency = 1.0f, float amplitude = 1.0f) {
		Noise2DBuffer(buffer, width, height, 1, glm::vec2(0), octaves, persistence, frequency, amplitude);
	}

	static void Noise2DBuffer(uint8_t* buffer, int width, int height, const glm::vec2& pos, int octaves = 1, float persistence = 1.0f, float frequency = 1.0f, float amplitude = 1.0f) {
		Noise2DBuffer(buffer, width, height, 1, pos, octaves, persistence, frequency, amplitude);
	}

	static void Noise2DGray(uint8_t* buffer, int width, int height, int octaves = 1, float persistence = 1.0f, float frequency = 1.0f, float amplitude = 1.0f) {
		Noise2DBuffer(buffer, width, height, 3, glm::vec2(0), octaves, persistence, frequency, amplitude);
	}

	static void Noise2DGray(uint8_t* buffer, int width, int height, const glm::vec2& pos, int octaves = 1, float persistence = 1.0f, float frequency = 1.0f, float amplitude = 1.0f) {
		Noise2DBuffer(buffer, width, height, 3, pos, noise::Simplex::Noise2D, octaves, persistence, frequency, amplitude);
	}

	static void Noise2DGrayA(uint8_t* buffer, int width, int height, int octaves = 1, float persistence = 1.0f, float frequency = 1.0f, float amplitude = 1.0f) {
		Noise2DBuffer(buffer, width, height, 4, glm::vec2(0), octaves, persistence, frequency, amplitude);
	}

	static void Noise2DGrayA(uint8_t* buffer, int width, int height, const glm::vec2& pos, int octaves = 1, float persistence = 1.0f, float frequency = 1.0f, float amplitude = 1.0f) {
		Noise2DBuffer(buffer, width, height, 4, pos, noise::Simplex::Noise2D, octaves, persistence, frequency, amplitude);
	}

	static void Noise2DRGB(uint8_t* buffer, int width, int height, int octaves = 1, float persistence = 1.0f, float frequency = 1.0f, float amplitude = 1.0f) {
		Noise2DChannel(buffer, width, height, 3, glm::vec2(0), octaves, persistence, frequency, amplitude);
	}

	static void Noise2DRGB(uint8_t* buffer, int width, int height, const glm::vec2& pos, int octaves = 1, float persistence = 1.0f, float frequency = 1.0f, float amplitude = 1.0f) {
		Noise2DChannel(buffer, width, height, 3, pos, octaves, persistence, frequency, amplitude);
	}

	static void Noise2DRGBA(uint8_t* buffer, int width, int height, int octaves = 1, float persistence = 1.0f, float frequency = 1.0f, float amplitude = 1.0f) {
		Noise2DChannel(buffer, width, height, 4, glm::vec2(0), octaves, persistence, frequency, amplitude);
	}

	static void Noise2DRGBA(uint8_t* buffer, int width, int height, const glm::vec2& pos, int octaves = 1, float persistence = 1.0f, float frequency = 1.0f, float amplitude = 1.0f) {
		Noise2DChannel(buffer, width, height, 4, pos, octaves, persistence, frequency, amplitude);
	}

	static void Noise2DChannel(uint8_t* buffer, int width, int height, int components, const glm::vec2& pos, int octaves = 1, float persistence = 1.0f, float frequency = 1.0f, float amplitude = 1.0f) {
		uint8_t bufferChannel[width * height];
		for (int channel = 0; channel < components; ++channel) {
			Noise2DBuffer(bufferChannel, width, height, 1, pos + glm::vec2(channel), noise::Simplex::Noise2D, octaves, persistence, frequency, amplitude);
			int index = 0;
			for (int x = 0; x < width; ++x) {
				for (int y = 0; y < height; ++y, ++index) {
					buffer[index * 3 + channel] = bufferChannel[index];
				}
			}
		}
	}

	template<class Func, class ... Args>
	static void Noise2DBuffer(uint8_t* buffer, int width, int height, int components, const glm::vec2& pos, Func&& func, Args&&... args) {
		for (int x = 0; x < width; ++x) {
			for (int y = 0; y < height; ++y) {
				float noise = func(glm::vec2(x, y) + pos, std::forward<Args>(args)...);
				float noiseHeight = (noise + 1.0f) * 0.5f;
				unsigned char color = (unsigned char) (noiseHeight * 255.0f);
				int index = y * (width * components) + (x * components);
				const int n = components == 4 ? 3 : components;
				for (int i = 0; i < n; ++i) {
					buffer[index++] = color;
				}
				if (components == 4) {
					buffer[index] = 255;
				}
			}
		}
	}

	static void SeamlessNoise2DBuffer(uint8_t* buffer, int size, int octaves = 1, float persistence = 1.0f, float frequency = 1.0f, float amplitude = 1.0f) {
		SeamlessNoise2DBuffer(buffer, size, 1, glm::vec4(0), noise::Simplex::Noise4D, octaves, persistence, frequency, amplitude);
	}

	static void SeamlessNoise2DBuffer(uint8_t* buffer, int size, const glm::vec4& pos, int octaves = 1, float persistence = 1.0f, float frequency = 1.0f, float amplitude = 1.0f) {
		SeamlessNoise2DBuffer(buffer, size, 1, pos, noise::Simplex::Noise4D, octaves, persistence, frequency, amplitude);
	}

	static void SeamlessNoise2DBuffer(uint8_t* buffer, int size, int components, int octaves = 1, float persistence = 1.0f, float frequency = 1.0f, float amplitude = 1.0f) {
		SeamlessNoise2DBuffer(buffer, size, components, glm::vec4(0), noise::Simplex::Noise4D, octaves, persistence, frequency, amplitude);
	}

	static void SeamlessNoise2DBuffer(uint8_t* buffer, int size, int components, const glm::vec4& pos, int octaves = 1, float persistence = 1.0f, float frequency = 1.0f, float amplitude = 1.0f) {
		SeamlessNoise2DBuffer(buffer, size, components, pos, noise::Simplex::Noise4D, octaves, persistence, frequency, amplitude);
	}

	static void SeamlessNoise2DGray(uint8_t* buffer, int size, int octaves = 1, float persistence = 1.0f, float frequency = 1.0f, float amplitude = 1.0f) {
		SeamlessNoise2DBuffer(buffer, size, 3, glm::vec4(0), noise::Simplex::Noise4D, octaves, persistence, frequency, amplitude);
	}

	static void SeamlessNoise2DGray(uint8_t* buffer, int size, const glm::vec4& pos, int octaves = 1, float persistence = 1.0f, float frequency = 1.0f, float amplitude = 1.0f) {
		SeamlessNoise2DBuffer(buffer, size, 3, pos, noise::Simplex::Noise4D, octaves, persistence, frequency, amplitude);
	}

	static void SeamlessNoise2DGrayA(uint8_t* buffer, int size, int octaves = 1, float persistence = 1.0f, float frequency = 1.0f, float amplitude = 1.0f) {
		SeamlessNoise2DBuffer(buffer, size, 4, glm::vec4(0), noise::Simplex::Noise4D, octaves, persistence, frequency, amplitude);
	}

	static void SeamlessNoise2DGrayA(uint8_t* buffer, int size, const glm::vec4& pos, int octaves = 1, float persistence = 1.0f, float frequency = 1.0f, float amplitude = 1.0f) {
		SeamlessNoise2DBuffer(buffer, size, 4, pos, noise::Simplex::Noise4D, octaves, persistence, frequency, amplitude);
	}

	static void SeamlessNoise2DRGB(uint8_t* buffer, int size, int octaves = 1, float persistence = 1.0f, float frequency = 1.0f, float amplitude = 1.0f) {
		SeamlessNoise2DChannel(buffer, size, 3, glm::vec4(0), octaves, persistence, frequency, amplitude);
	}

	static void SeamlessNoise2DRGB(uint8_t* buffer, int size, const glm::vec4& pos, int octaves = 1, float persistence = 1.0f, float frequency = 1.0f, float amplitude = 1.0f) {
		SeamlessNoise2DChannel(buffer, size, 3, pos, octaves, persistence, frequency, amplitude);
	}

	static void SeamlessNoise2DRGBA(uint8_t* buffer, int size, int octaves = 1, float persistence = 1.0f, float frequency = 1.0f, float amplitude = 1.0f) {
		SeamlessNoise2DChannel(buffer, size, 4, glm::vec4(0), octaves, persistence, frequency, amplitude);
	}

	static void SeamlessNoise2DRGBA(uint8_t* buffer, int size, const glm::vec4& pos, int octaves = 1, float persistence = 1.0f, float frequency = 1.0f, float amplitude = 1.0f) {
		SeamlessNoise2DChannel(buffer, size, 4, pos, octaves, persistence, frequency, amplitude);
	}

	static void SeamlessNoise2DChannel(uint8_t* buffer, int size, int components, const glm::vec4& pos, int octaves = 1, float persistence = 1.0f, float frequency = 1.0f, float amplitude = 1.0f) {
		uint8_t bufferChannel[size * size];
		for (int channel = 0; channel < components; ++channel) {
			SeamlessNoise2DBuffer(bufferChannel, size, 1, pos + glm::vec4(channel), noise::Simplex::Noise4D, octaves, persistence, frequency, amplitude);
			int index = 0;
			for (int x = 0; x < size; ++x) {
				for (int y = 0; y < size; ++y, ++index) {
					buffer[index * 3 + channel] = bufferChannel[index];
				}
			}
		}
	}

	template<class Func, class ... Args>
	static void SeamlessNoise2DBuffer(uint8_t* buffer, int size, int components, const glm::vec4& pos, Func&& func, Args&&... args) {
		// seamless noise: http://www.gamedev.net/blog/33/entry-2138456-seamless-noise/
		const float pi2 = glm::two_pi<float>();
		const float d = 1.0f / size;
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
				float noise = func(glm::vec4(nx, ny, nz, nw) + pos, std::forward<Args>(args)...);
				noise = glm::clamp(noise::norm(noise), 0.0f, 1.0f);
				const unsigned char color = (unsigned char) (noise * 255.0f);
				int index = y * (size * components) + (x * components);
				const int fillComponents = components == 4 ? 3 : components;
				for (int i = 0; i < fillComponents; ++i) {
					buffer[index++] = color;
				}
				if (components == 4) {
					buffer[index] = 255;
				}
			}
		}
	}

	static float ScaledNoise2D(const float lowerBound, const float upperBound, const glm::vec2& pos) {
		return Noise2D(pos) * (upperBound - lowerBound) / 2.0f + (upperBound + lowerBound) / 2.0f;
	}

	static float ScaledNoise3D(const float lowerBound, const float upperBound, const glm::vec3& pos) {
		return Noise3D(pos) * (upperBound - lowerBound) / 2.0f + (upperBound + lowerBound) / 2.0f;
	}

	static float ScaledNoise4D(const float lowerBound, const float upperBound, const glm::vec4& pos) {
		return Noise4D(pos) * (upperBound - lowerBound) / 2.0f + (upperBound + lowerBound) / 2.0f;
	}
};

}
