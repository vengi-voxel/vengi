#pragma once

#include "core/GLM.h"
#include "core/Rect.h"
#include "Camera.h"
#include "DepthBuffer.h"

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

	void init(const glm::vec3& sunDirection, const glm::ivec2& position, const glm::ivec2& dimension);
	glm::vec3 init(float sunTheta, float sunPhi, const glm::ivec2& position, const glm::ivec2& dimension);

	const Camera& camera() const;

	void update(long dt, const Camera& camera);

	glm::vec3 direction() const;
	void setDirection(const glm::vec3& direction);

	/**
	 * @brief Because we're modeling a directional light source all its light rays are parallel.
	 * For this reason we're going to use an orthographic projection matrix for the light
	 * source where there is no perspective deform
	 */
	const glm::mat4& projectionMatrix() const;

	glm::mat4 viewProjectionMatrix(const Camera& camera) const;

	glm::ivec2 dimension() const;

	const glm::mat4& viewMatrix() const;
};

inline void SunLight::setDirection(const glm::vec3& sunDirection) {
	_sunCamera.setPosition(-sunDirection);
	_sunCamera.lookAt(glm::zero<glm::vec3>(), glm::up);
}

inline glm::vec3 SunLight::direction() const {
	return _sunCamera.direction();
}

inline glm::ivec2 SunLight::dimension() const {
	return _sunCamera.dimension();
}

inline const glm::mat4& SunLight::projectionMatrix() const {
	return _sunCamera.projectionMatrix();
}

inline glm::mat4 SunLight::viewProjectionMatrix(const Camera& camera) const {
	return glm::translate(projectionMatrix() * viewMatrix(), -camera.position());
}

inline const glm::mat4& SunLight::viewMatrix() const {
	return _sunCamera.viewMatrix();
}

inline const video::Camera& SunLight::camera() const {
	return _sunCamera;
}

}
