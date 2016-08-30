#pragma once

#include "core/GLM.h"
#include "core/Rect.h"
#include "Camera.h"

namespace video {

class SunLight {
private:
	class SunCamera : public Camera {
	public:
		void updateSun(long deltaFrame, const core::RectFloat& bbox) {
			_dirty |= DIRTY_PERSPECTIVE;
			updateOrientation();
			updateViewMatrix();
			// normalize the opengl depth from [-1, 1] to [0, 1]
			static const glm::mat4 normalizeDepth = glm::scale(glm::translate(glm::mat4(), glm::backward), glm::vec3(1.0f, 1.0f, 0.5f));
			_projectionMatrix = normalizeDepth * glm::ortho(bbox.getMinX(), bbox.getMaxX(), bbox.getMinZ(), bbox.getMaxZ(), nearPlane(), farPlane());
			updateFrustumPlanes();
			updateFrustumVertices();
			_dirty = 0;
		}
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

	glm::mat4 viewProjectionMatrix() const;

	const glm::mat4& viewMatrix() const;
};

}

inline glm::vec3 video::SunLight::direction() const {
	return _sunCamera.direction();
}

inline const glm::mat4& video::SunLight::projectionMatrix() const {
	return _sunCamera.projectionMatrix();
}

inline glm::mat4 video::SunLight::viewProjectionMatrix() const {
	return projectionMatrix() * viewMatrix();
}

inline const glm::mat4& video::SunLight::viewMatrix() const {
	return _sunCamera.viewMatrix();
}

inline const video::Camera& video::SunLight::camera() const {
	return _sunCamera;
}
