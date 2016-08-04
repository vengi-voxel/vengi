#include "SunLight.h"

namespace video {

void SunLight::update(long dt, const Camera& camera) {
	// normalize the opengl depth from [-1, 1] to [0, 1]
	_lightProjection = glm::translate(glm::mat4(), glm::backward);
	_lightProjection = glm::scale(_lightProjection, glm::vec3(1.0f, 1.0f, 0.5f));

	// TODO: calculate the obb around the frustum - but we have to improve later anyway for cascaded shadowmaps.
	// TODO: then create the ortho matrix from it. http://www.ogldev.org/www/tutorial49/tutorial49.html
	_lightProjection = _lightProjection * glm::ortho(-250.0f, +250.0f, -250.0f, +250.0f, 1.0f, 400.0f);
	_lightView = glm::lookAt(_sunPos, glm::zero<glm::vec3>(), glm::up);
	const glm::mat4& lightModel = glm::translate(glm::mat4(1.0f), camera.position());
	_lightSpaceMatrix = _lightProjection * _lightView * lightModel;
	_lightDir = glm::vec3(glm::column(glm::inverse(_lightView), 2));
}

}
