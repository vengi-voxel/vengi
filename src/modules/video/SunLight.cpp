#include "SunLight.h"

namespace video {

void SunLight::update(long dt, const Camera& camera, const core::RectFloat& sceneBoudingBox) {
	_sunCamera.setNearPlane(1.0f);
	_sunCamera.setFarPlane(400.0f);
	_sunCamera.init(camera.dimension());
	_sunCamera.lookAt(glm::zero<glm::vec3>());
	_sunCamera.updateSun(dt, sceneBoudingBox);
}

}
