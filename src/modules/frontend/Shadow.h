#pragma once

#include "video/Camera.h"
#include "core/Common.h"
#include <vector>

namespace frontend {

class Shadow {
private:
	glm::vec3 _sunDirection;
	glm::mat4 _lightView;
	std::vector<glm::mat4> _cascades;
	std::vector<float> _distances;

public:
	bool init();

	void calculateShadowData(const video::Camera& camera, bool active, int maxDepthBuffers, const glm::ivec2& depthBufferSize);

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

}
