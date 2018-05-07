/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "Simplex.h"
#include <glm/glm.hpp>
#include <stdint.h>

namespace compute {
class NoiseShader;
}

namespace noise {

/**
 * @brief Normalizes a noise value in the range [-1,-1] to [0,1]
 */
inline float norm(float noise) {
	return (glm::clamp(noise, -1.0f, 1.0f) + 1.0f) * 0.5f;
}

/**
 * @brief Wrapper class that picks the best path to calculate the noise. This is either on the gpu or the cpu.
 *
 * fBM (fractional Brownian motion) is a composite Perlin noise algorithm. It creates more turbulence with more octaves.
 *
 * To cover all possible scales, the octaves are typically a bit less than @code log(width) / log(lacunarity)@endcode.
 * So, for a 1024x1024 heightfield, about 10 octaves are needed. The persistence influences the terrain turbulence.
 *
 * The amount of octaves controls the level of detail. Adding more octaves increases the detail level, but also the computation time.
 * The persistence is a multiplier that defines how fast the amplitude diminishes for each successive octave.
 * The lacunarity is a multiplier that defines how quickly the frequency changes for each successive octave.
 * The amplitude is the maximum absolute value that the noise function can output.
 */
class Noise : public core::IComponent {
private:
	compute::NoiseShader& _shader;
	bool _useShader = false;
	bool _enableShader = true;
public:
	Noise();
	~Noise();

	bool canUseShader() const;
	/**
	 * @return @c true if the value was set, @c false otherwise. This might happen if you e.g. try to activate noise shaders,
	 * but your system doesn't support them.
	 */
	bool useShader(bool enableShader);

	bool init() override;
	void shutdown() override;

	/**
	 * @brief Fills the given target buffer with RGB values for the noise.
	 * @param[in] buffer pointer to the target buffer - must be of size @c width * height * 3
	 * @param[in] size the width and height of the image. Make sure that the target buffer has enough space to
	 * store the needed data for the dimensions you specify here.
	 * @param[in] octaves the amount of noise calls that contribute to the final result
	 * @param[in] persistence the persistence defines how much of the amplitude will be applied to the next noise call (only makes
	 * sense if you have @c octaves > 1). The higher this value is (ranges from 0-1) the more each new octave will add to the result.
	 * @param[in] frequency the higher the @c frequency the more deviation you get in your noise (wavelength).
	 * @param[in] amplitude the amplitude defines how high the noise will be.
	 */
	void seamlessNoise(uint8_t* buffer, int size, int octaves = 1, float persistence = 1.0f, float frequency = 1.0f, float amplitude = 1.0f) const;

	/**
	 * @return Range [-+2147483647,+2147483647].
	 */
	int32_t intValueNoise(const glm::ivec3& pos, int32_t seed = 0) const;

	/**
	 * @return Range [-1.0,+1.0].
	 */
	double doubleValueNoise(const glm::ivec3& pos, int32_t seed = 0) const;

	double voronoi(const glm::dvec3& pos, bool enableDistance, double frequency = 1.0, int seed = 0) const;

	/**
	 * @param lacunarity spacing between successive octaves (use exactly 2.0 for wrapping output)
	 * @param gain relative weighting applied to each successive octave
	 */
	float swissTurbulence(const glm::vec2& p, float offset, int octaves, float lacunarity = 2.0f, float gain = 0.6f, float warp = 0.15f) const;

	/**
	 * @param lacunarity spacing between successive octaves (use exactly 2.0 for wrapping output)
	 * @param gain relative weighting applied to each successive octave
	 */
	float jordanTurbulence(const glm::vec2&p, float offset, int octaves, float lacunarity = 2.0f, float gain1 = 0.8f, float gain = 0.5f, float warp0 = 0.4f, float warp = 0.35f,
			float damp0 = 1.0f, float damp = 0.8f, float damp_scale = 1.0f) const;

	/**
	 * @brief Transforms the latitude and longitude unit sphere coordinates into the
	 * cartesian coordinates and uses those as input for the noise function.
	 * @param[in] latitude Given in degrees - must be [-90,90]
	 * @param[in] longitude Given in degrees - must be [-180,180]
	 */
	float sphereNoise(float longitude, float latitude);
};

inline bool Noise::canUseShader() const {
	return _useShader && _enableShader;
}

}
