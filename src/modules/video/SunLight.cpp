#include "SunLight.h"

namespace video {

void SunLight::update(long dt, const Camera& camera, const glm::vec4& sceneBoudingBox) {
	// normalize the opengl depth from [-1, 1] to [0, 1]
	_lightProjection = glm::translate(glm::mat4(), glm::backward);
	_lightProjection = glm::scale(_lightProjection, glm::vec3(1.0f, 1.0f, 0.5f));

	/*
	 * TODO: calculate the obb around the frustum - but we have to improve later anyway for cascaded shadowmaps.
	 * TODO: then create the ortho matrix from it. http://www.ogldev.org/www/tutorial49/tutorial49.html
	 *
	 * Potential Shadow Receivers, or PSRs for short, are objects which belong at the same time to the light frustum,
	 * to the view frustum, and to the scene bounding box. As their name suggest,
	 * these objects are susceptible to be shadowed : they are visible by the camera and by the light.
	 *
	 * Potential Shadow Casters, or PCFs, are all the Potential Shadow Receivers, plus all objects which lie between
	 * them and the light (an object may not be visible but still cast a visible shadow).
	 *
	 * So, to compute the light projection matrix, take all visible objects, remove those which are too far away,
	 * and compute their bounding box; Add the objects which lie between this bounding box and the light,
	 * and compute the new bounding box (but this time, aligned along the light direction).
	 *
	 * Precise computation of these sets involve computing convex hulls intersections,
	 * but this method is much easier to implement.
	 *
	 * This method will result in popping when objects disappear from the frustum, because the shadowmap resolution will
	 * suddenly increase. Cascaded Shadow Maps don’t have this problem, but are harder to implement, and you can still
	 * compensate by smoothing the values over time.
	 *
	 * SDL_Rect has a lot of the needed functionality.
	 */

	_lightProjection = _lightProjection * glm::ortho(sceneBoudingBox.x, sceneBoudingBox.y, sceneBoudingBox.z, sceneBoudingBox.w, 1.0f, 400.0f);
	_lightView = glm::lookAt(_sunPos, glm::zero<glm::vec3>(), glm::up);
	const glm::mat4& lightModel = glm::translate(glm::mat4(1.0f), camera.position());
	_lightSpaceMatrix = _lightProjection * _lightView * lightModel;
	_lightDir = glm::vec3(glm::column(glm::inverse(_lightView), 2));
}

}
