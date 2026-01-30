/**
 * @file
 */

#include "Shadow.h"
#include "video/Camera.h"
#include "core/GLM.h"
#include "video/Trace.h"
#include "core/Var.h"
#include "core/collection/Array.h"
#include "video/Renderer.h"
#include "core/Log.h"
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/norm.hpp>

namespace voxelrender {

Shadow::~Shadow() {
	core_assert_msg(_parameters.maxDepthBuffers == -1, "Shadow::shutdown() wasn't called");
}

bool Shadow::init(const ShadowParameters& parameters) {
	if (parameters.maxDepthBuffers < 1) {
		return false;
	}
	if (parameters.maxDepthBuffers > shader::VoxelShaderConstants::getMaxDepthBuffers()) {
		return false;
	}
	_parameters = parameters;
	const glm::vec3 sunPos(25.0f, 100.0f, 25.0f);
	setPosition(sunPos, glm::vec3(0.0f), glm::up());

	const glm::ivec2 smSize(core::Var::getSafe(cfg::ClientShadowMapSize)->intVal());
	const video::FrameBufferConfig& cfg = video::defaultDepthBufferConfig(smSize, _parameters.maxDepthBuffers);
	if (!_depthBuffer.init(cfg)) {
		Log::error("Failed to init the depthbuffer");
		return false;
	}

	return true;
}

void Shadow::shutdown() {
	_depthBuffer.shutdown();
	_parameters = ShadowParameters();
}

static inline float getBoundingSphereRadius(const glm::vec3& center, const core::Array<glm::vec3, 8>& points) {
	float radius = 0.0f;
	for (size_t i = 0; i < points.size(); ++i) {
		const glm::vec3& p = points[i];
		radius = core_max(radius, glm::distance2(center, p));
	}
	return glm::sqrt(radius);
}

glm::vec4 Shadow::splitFrustumSphereBoundingBox(const video::Camera& camera, float near, float far) const {
	const glm::mat4& projection = camera.projectionMatrix();
	const glm::mat4& inverseProjection = camera.inverseProjectionMatrix();

	const float znearp = glm::project(projection, glm::vec3(0.0f, 0.0f, -near)).z;
	const float zfarp = glm::project(projection, glm::vec3(0.0f, 0.0f, -far)).z;

	core::Array<glm::vec3, 8> points;

	int idx = 0;
	for (int x = 0; x < 2; ++x) {
		for (int y = 0; y < 2; ++y) {
			for (int z = 0; z < 2; ++z) {
				const glm::vec3 v(x ? 1 : -1, y ? 1 : -1, z ? zfarp : znearp);
				const glm::vec3& p = glm::project(inverseProjection, v);
				points[idx++] = p;
			}
		}
	}

	const glm::vec3& begin = glm::project(inverseProjection, glm::vec3(0.0f, 0.0f, znearp));
	const glm::vec3& end = glm::project(inverseProjection, glm::vec3(0.0f, 0.0f, zfarp));
	float radiusBegin = getBoundingSphereRadius(begin, points);
	float radiusEnd = getBoundingSphereRadius(end, points);

	float rangeBegin = 0.0f;
	float rangeEnd = 1.0f;

	while (rangeEnd - rangeBegin > 1e-3) {
		const float rangeMiddle = (rangeBegin + rangeEnd) / 2.0f;
		const float radiusMiddle = getBoundingSphereRadius(glm::mix(begin, end, rangeMiddle), points);

		if (radiusBegin < radiusEnd) {
			radiusEnd = radiusMiddle;
			rangeEnd = rangeMiddle;
		} else {
			radiusBegin = radiusMiddle;
			rangeBegin = rangeMiddle;
		}
	}

	return glm::vec4(glm::mix(begin, end, rangeBegin), radiusBegin);
}

void Shadow::update(const video::Camera& camera, bool active) {
	core_trace_scoped(ShadowCalculate);
	_shadowRangeZ = camera.farPlane() * 3.0f;

	if (!active) {
		for (int i = 0; i < _parameters.maxDepthBuffers; ++i) {
			_cascades[i] = glm::mat4(1.0f);
			_distances[i] = camera.farPlane();
		}
		return;
	}

	const glm::vec2 &dim = dimension();
	float planes[shader::VoxelShaderConstants::getMaxDepthBuffers() * 2];
	camera.sliceFrustum(planes, _parameters.maxDepthBuffers * 2, _parameters.maxDepthBuffers, _parameters.sliceWeight);
	const glm::mat4 &inverseView = camera.inverseViewMatrix();
	const glm::mat4 &inverseLightView = _lightView * inverseView;

	for (int i = 0; i < _parameters.maxDepthBuffers; ++i) {
		const float near = planes[i * 2 + 0];
		const float far = planes[i * 2 + 1];
		const glm::vec4 &sphere = splitFrustumSphereBoundingBox(camera, near, far);
		const glm::vec3 lightCenter(inverseLightView * glm::vec4(sphere.x, sphere.y, sphere.z, 1.0f));
		const float lightRadius = sphere.w;

		// round to prevent movement
		const float xRound = lightRadius * 2.0f / dim.x;
		const float yRound = lightRadius * 2.0f / dim.y;
		const float zRound = 1.0f;
		const glm::vec3 round(xRound, yRound, zRound);
		const glm::vec3 lightCenterRounded = glm::round(lightCenter / round) * round;
		const glm::mat4& lightProjection = glm::ortho(
				 lightCenterRounded.x - lightRadius,
				 lightCenterRounded.x + lightRadius,
				 lightCenterRounded.y - lightRadius,
				 lightCenterRounded.y + lightRadius,
				-lightCenterRounded.z - (_shadowRangeZ - lightRadius),
				-lightCenterRounded.z + lightRadius);
		_cascades[i] = lightProjection * _lightView;
		_distances[i] = far;
	}
}

bool Shadow::bind(video::TextureUnit unit) {
	const bool state = video::bindTexture(unit, _depthBuffer, video::FrameBufferAttachment::Depth);
	core_assert(state);
	return state;
}

void Shadow::render(const funcRender& renderCallback, bool clearDepthBuffer) {
	video_trace_scoped(ShadowRender);
	const bool oldBlend = video::disable(video::State::Blend);
	// Enable front face culling to reduce shadow acne (render back faces into shadow map)
	video::enable(video::State::CullFace);
	video::cullFace(video::Face::Front);

	// Enable polygon offset to help with depth precision issues
	// This provides a hardware-assisted constant and slope-scaled depth bias
	video::enable(video::State::PolygonOffsetFill);
	// factor: slope-scaled bias (helps with surfaces at angles)
	// units: constant bias (helps with depth buffer precision)
	video::polygonOffset(glm::vec2(1.1f, 4.0f));

	video::colorMask(false, false, false, false);
	_depthBuffer.bind(false);
	for (int i = 0; i < _parameters.maxDepthBuffers; ++i) {
		_depthBuffer.bindTextureAttachment(video::FrameBufferAttachment::Depth, i, clearDepthBuffer);
		if (!renderCallback(i, _cascades[i])) {
			break;
		}
	}
	_depthBuffer.unbind();
	video::colorMask(true, true, true, true);

	// Restore polygon offset state
	video::polygonOffset(glm::vec2(0.0f));
	video::disable(video::State::PolygonOffsetFill);

	video::cullFace(video::Face::Back);
	if (oldBlend) {
		video::enable(video::State::Blend);
	}
}

const glm::ivec2& Shadow::dimension() const {
	return _depthBuffer.dimension();
}

void Shadow::setPosition(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up) {
	setLightViewMatrix(glm::lookAt(eye, center, up));
}

void Shadow::setLightViewMatrix(const glm::mat4& lightView) {
	_lightView = lightView;
	//_sunDirection = normalize(center - sunPos);
	_sunDirection = glm::vec3(glm::column(glm::inverse(_lightView), 2));
}

glm::vec3 Shadow::sunPosition() const {
	const glm::mat3 rotMat(_lightView);
	const glm::vec3 d(_lightView[3]);
	return -d * rotMat;
}

}
