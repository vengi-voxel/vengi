/**
 * @file
 */

#include "Shadow.h"
#include "video/Camera.h"
#include "core/GLM.h"
#include "core/Trace.h"

namespace render {

bool Shadow::init() {
	const float length = 50.0f;
	const glm::vec3 sunPos(length, length, -length);
	const glm::vec3 center(0.0f);
	_lightView = glm::lookAt(sunPos, center, glm::up);
	//_sunDirection = normalize(center - sunPos);
	_sunDirection = glm::vec3(glm::column(glm::inverse(_lightView), 2));
	return true;
}

void Shadow::shutdown() {
}

void Shadow::calculateShadowData(const video::Camera& camera, bool active, int maxDepthBuffers, const glm::ivec2& depthBufferSize, float sliceWeight) {
	core_trace_scoped(ShadowCalculate);
	_cascades.resize(maxDepthBuffers);
	_distances.resize(maxDepthBuffers);
	_depthDimension = depthBufferSize;

	if (!active) {
		for (int i = 0; i < maxDepthBuffers; ++i) {
			_cascades[i] = glm::mat4(1.0f);
			_distances[i] = camera.farPlane();
		}
		return;
	}

	std::vector<float> planes(maxDepthBuffers * 2);
	camera.sliceFrustum(&planes.front(), planes.size(), maxDepthBuffers, sliceWeight);
	const glm::mat4& inverseView = camera.inverseViewMatrix();
	const float shadowRangeZ = camera.farPlane() * 3.0f;

	for (int i = 0; i < maxDepthBuffers; ++i) {
		const float near = planes[i * 2 + 0];
		const float far = planes[i * 2 + 1];
		const glm::vec4& sphere = camera.splitFrustumSphereBoundingBox(near, far);
		const glm::vec3 lightCenter(_lightView * inverseView * glm::vec4(sphere.x, sphere.y, sphere.z, 1.0f));
		const float lightRadius = sphere.w;

		// round to prevent movement
		const float xRound = lightRadius * 2.0f / depthBufferSize.x;
		const float yRound = lightRadius * 2.0f / depthBufferSize.y;
		const float zRound = 1.0f;
		const glm::vec3 round(xRound, yRound, zRound);
		const glm::vec3 lightCenterRounded = glm::round(lightCenter / round) * round;
		const glm::mat4& lightProjection = glm::ortho(
				 lightCenterRounded.x - lightRadius,
				 lightCenterRounded.x + lightRadius,
				 lightCenterRounded.y - lightRadius,
				 lightCenterRounded.y + lightRadius,
				-lightCenterRounded.z - (shadowRangeZ - lightRadius),
				-lightCenterRounded.z + lightRadius);
		_cascades[i] = lightProjection * _lightView;
		_distances[i] = far;
	}
}

}
