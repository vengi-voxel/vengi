#include "SunLight.h"
#include "core/AABB.h"

namespace video {

SunLight::SunLight() {
	_sunCamera.setNearPlane(1.0f);
	_sunCamera.setFarPlane(400.0f);
}

void SunLight::init(const glm::vec3& sunPos, const glm::ivec2& dimension) {
	_sunCamera.init(dimension);
	_sunCamera.setPosition(sunPos);
	_sunCamera.lookAt(glm::zero<glm::vec3>());
}

void SunLight::update(long dt, const Camera& camera) {
	glm::vec3 out[video::FRUSTUM_VERTICES_MAX];
	camera.frustumCorners(out, nullptr);

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
	core::AABB<float> aabb(out[0], out[1]);
	for (int i = 0; i < video::FRUSTUM_VERTICES_MAX; ++i) {
		aabb.accumulate(out[i]);
	}
	const core::RectFloat sceneBoundingBox(aabb.getLowerX(), aabb.getLowerZ(), aabb.getUpperX(), aabb.getUpperZ());
	_sunCamera.updateSun(dt, sceneBoundingBox);
}

}
