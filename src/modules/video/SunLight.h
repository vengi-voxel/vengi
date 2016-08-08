#pragma once

#include "core/GLM.h"
#include "Camera.h"

namespace video {

class SunLight {
private:
	glm::vec3 _sunPos;
	glm::mat4 _lightProjection;
	glm::mat4 _lightView;
	glm::mat4 _lightSpaceMatrix;
	glm::vec3 _lightDir;

public:
	void update(long dt, const Camera& camera, const glm::vec4& sceneBoudingBox);

	const glm::vec3& dir() const;

	/**
	 * @brief Because we're modeling a directional light source all its light rays are parallel.
	 * For this reason we're going to use an orthographic projection matrix for the light
	 * source where there is no perspective deform
	 */
	const glm::mat4& projection() const;

	const glm::mat4& model() const;

	const glm::mat4& view() const;

	const glm::vec3& pos() const;

	void setPos(const glm::vec3& sunPos);
};

}

inline const glm::vec3& video::SunLight::dir() const {
	return _lightDir;
}

inline const glm::mat4& video::SunLight::projection() const {
	return _lightProjection;
}

inline const glm::mat4& video::SunLight::model() const {
	return _lightSpaceMatrix;
}

inline const glm::mat4& video::SunLight::view() const {
	return _lightView;
}

inline const glm::vec3& video::SunLight::pos() const {
	return _sunPos;
}

inline void video::SunLight::setPos(const glm::vec3& sunPos) {
	_sunPos = sunPos;
}
