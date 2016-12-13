#include "SunLight.h"
#include "core/AABB.h"

namespace video {

void SunLight::SunCamera::updateSun(long deltaFrame, const core::RectFloat& bbox) {
	_dirty |= DIRTY_PERSPECTIVE;
	updateOrientation();
	updateViewMatrix();
	// normalize the opengl depth from [-1, 1] to [0, 1]
	static const glm::mat4 normalizeDepth = glm::scale(glm::translate(glm::mat4(), glm::backward), glm::vec3(1.0f, 1.0f, 0.5f));
	_projectionMatrix = normalizeDepth * glm::ortho(bbox.getMinX(), bbox.getMaxX(), bbox.getMinZ(), bbox.getMaxZ(), nearPlane(), farPlane());
	updateFrustumPlanes();
	updateFrustumVertices();
	_viewProjectionMatrix = projectionMatrix() * viewMatrix();
	_dirty = 0;
}

SunLight::SunLight() {
	_sunCamera.setNearPlane(1.0f);
	_sunCamera.setFarPlane(400.0f);
}

static inline glm::vec3 spherical(float theta, float phi) {
	return glm::vec3(glm::cos(phi) * glm::sin(theta), glm::sin(phi) * glm::sin(theta), glm::cos(theta));
}

glm::vec3 SunLight::init(float sunTheta, float sunPhi, const glm::ivec2& position, const glm::ivec2& dimension) {
	const glm::vec3& sunDirection = -spherical(glm::radians(sunTheta), glm::radians(sunPhi));
	_sunCamera.init(glm::vec3(), dimension);
	const glm::vec3& lightUp = glm::abs(sunDirection.z) > 0.7f ? glm::up : glm::backward;
	const glm::quat& quat = glm::quat_cast(glm::lookAt(glm::vec3(), sunDirection, lightUp));
	_sunCamera.setQuaternion(quat);
	return sunDirection;
}

void SunLight::init(const glm::vec3& sunDirection, const glm::ivec2& position, const glm::ivec2& dimension) {
	core_assert(sunDirection != glm::zero<glm::vec3>());
	_sunCamera.init(position, dimension);
	setDirection(sunDirection);
}

void SunLight::update(long dt, const Camera& camera) {
	core::AABB<float> aabb = camera.aabb();
	aabb.shift(-aabb.getCenter());

	_sunCamera.setFarPlane(camera.farPlane());
	_sunCamera.setNearPlane(camera.nearPlane());

	const core::RectFloat sceneBoundingBox(aabb.getLowerX(), aabb.getLowerZ(), aabb.getUpperX(), aabb.getUpperZ());
	_sunCamera.updateSun(dt, sceneBoundingBox);
}

}
