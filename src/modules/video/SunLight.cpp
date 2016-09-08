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
	_dirty = 0;
}

SunLight::SunLight() {
	_sunCamera.setNearPlane(1.0f);
	_sunCamera.setFarPlane(400.0f);
}

void SunLight::init(const glm::vec3& sunDirection, const glm::ivec2& dimension) {
	core_assert(sunDirection != glm::zero<glm::vec3>());
	_sunCamera.init(dimension);
	_sunCamera.setPosition(glm::zero<glm::vec3>());
	const glm::vec3& invSunDirection = glm::vec3(0.5f, 2.0f, 2.0f);
	_sunCamera.lookAt(invSunDirection, glm::up);
}

void SunLight::update(long dt, const Camera& camera) {
	core::AABB<float> aabb = camera.aabb();
	aabb.shift(-aabb.getCenter());

	_sunCamera.setFarPlane(camera.farPlane());
	_sunCamera.setNearPlane(camera.nearPlane());

	/**
	 * https://www.uni-koblenz.de/~cg/Studienarbeiten/ShadowMappingNicoHempe.pdf
	 */

	/**
	 * @TODO:
	 * Transform the frustum corners to a space aligned with the shadow map axes.
	 * This would commonly be the directional light object's local space. (In fact,
	 * steps 1 and 2 can be done in one step by combining the inverse view-projection
	 * matrix of the camera with the inverse world matrix of the light.)
	 */
	const core::RectFloat sceneBoundingBox(aabb.getLowerX(), aabb.getLowerZ(), aabb.getUpperX(), aabb.getUpperZ());
	_sunCamera.updateSun(dt, sceneBoundingBox);
}

}
