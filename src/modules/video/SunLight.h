#pragma once

#include "core/GLM.h"
#include "core/Rect.h"
#include "Camera.h"

namespace video {

class SunLight {
private:
	class SunCamera : public Camera {
	public:
		void updateSun(long deltaFrame, const core::RectFloat& bbox);
	};

	SunCamera _sunCamera;

public:
	SunLight();

	void init(const glm::vec3& sunDirection, const glm::ivec2& dimension);

	const Camera& camera() const;

	void update(long dt, const Camera& camera);

	glm::vec3 direction() const;

	/**
	 * @brief Because we're modeling a directional light source all its light rays are parallel.
	 * For this reason we're going to use an orthographic projection matrix for the light
	 * source where there is no perspective deform
	 */
	const glm::mat4& projectionMatrix() const;

	glm::mat4 viewProjectionMatrix(const Camera& camera) const;

	const glm::mat4& viewMatrix() const;
};

}

inline glm::vec3 video::SunLight::direction() const {
	return _sunCamera.direction();
}

inline const glm::mat4& video::SunLight::projectionMatrix() const {
	return _sunCamera.projectionMatrix();
}

inline glm::mat4 video::SunLight::viewProjectionMatrix(const Camera& camera) const {
	return glm::translate(projectionMatrix() * viewMatrix(), -camera.position());
}

inline const glm::mat4& video::SunLight::viewMatrix() const {
	return _sunCamera.viewMatrix();
}

inline const video::Camera& video::SunLight::camera() const {
	return _sunCamera;
}
