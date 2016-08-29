#include "SunLight.h"
#include "core/AABB.h"

namespace video {

SunLight::SunLight() {
	_sunCamera.setNearPlane(1.0f);
	_sunCamera.setFarPlane(400.0f);
}

void SunLight::init(const glm::vec3& sunPos, const glm::vec3& sunDirection, const glm::ivec2& dimension) {
	_sunCamera.init(dimension);
	_sunCamera.setPosition(sunPos);
	_sunCamera.lookAt(sunPos + sunDirection, glm::up);
}

void SunLight::update(long dt, const Camera& camera) {
	const core::AABB<float>& aabb = camera.aabb();

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
