/**
 * @file
 */

#pragma once

#include <glm/fwd.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <vector>

namespace video {
class Camera;
}

namespace render {

/**
 * @brief Helper class for calculating the cascaded shadow map data
 */
class Shadow {
private:
	glm::vec3 _sunDirection;
	glm::mat4 _lightView;
	std::vector<glm::mat4> _cascades;
	std::vector<float> _distances;
	glm::vec2 _depthDimension;

public:
	bool init();
	void shutdown();

	const glm::vec2& depthDimension() const;
	/**
	 * @param[in] maxDepthBuffers the amount of cascades
	 */
	void calculateShadowData(const video::Camera& camera, bool active, int maxDepthBuffers, const glm::ivec2& depthBufferSize, float sliceWeight = 1.0f);

	const std::vector<glm::mat4>& cascades() const;
	const std::vector<float>& distances() const;
	const glm::vec3& sunDirection() const;
};

inline const std::vector<glm::mat4>& Shadow::cascades() const {
	return _cascades;
}

inline const std::vector<float>& Shadow::distances() const {
	return _distances;
}

inline const glm::vec3& Shadow::sunDirection() const {
	return _sunDirection;
}

inline const glm::vec2& Shadow::depthDimension() const {
	return _depthDimension;
}
}
