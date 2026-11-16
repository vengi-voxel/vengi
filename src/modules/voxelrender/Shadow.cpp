/**
 * @file
 */

#include "Shadow.h"
#include "math/Frustum.h"
#include "video/Camera.h"
#include "core/GLM.h"
#include "video/Trace.h"
#include "core/Var.h"
#include "core/Log.h"
#include "video/Renderer.h"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>

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

	const glm::ivec2 smSize(core::getVar(cfg::ClientShadowMapSize)->intVal());
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

namespace {

void lightSpaceAABB(const glm::mat4 &lightView, const glm::vec3 *worldPts, int n, glm::vec3 &mins, glm::vec3 &maxs) {
	mins = glm::vec3(1.0e8f);
	maxs = glm::vec3(-1.0e8f);
	for (int i = 0; i < n; ++i) {
		const glm::vec3 lp(lightView * glm::vec4(worldPts[i], 1.0f));
		mins = glm::min(mins, lp);
		maxs = glm::max(maxs, lp);
	}
}

void worldAABBToLightSpace(const glm::mat4 &lightView, const glm::vec3 &wmins, const glm::vec3 &wmaxs, glm::vec3 &mins,
						   glm::vec3 &maxs) {
	glm::vec3 pts[8];
	for (int i = 0; i < 8; ++i) {
		pts[i] = glm::vec3((i & 1) ? wmaxs.x : wmins.x, (i & 2) ? wmaxs.y : wmins.y, (i & 4) ? wmaxs.z : wmins.z);
	}
	lightSpaceAABB(lightView, pts, 8, mins, maxs);
}

glm::mat4 lightOrtho(const glm::mat4 &lightView, const glm::vec3 &lightMins, const glm::vec3 &lightMaxs,
					 const glm::ivec2 &dim) {
	glm::vec3 center = (lightMins + lightMaxs) * 0.5f;
	const glm::vec3 extent = (lightMaxs - lightMins) * 0.5f;
	float radius = core_max(extent.x, extent.y);
	radius = core_max(radius, 8.0f);
	radius += 2.0f;

	if (dim.x > 0 && dim.y > 0) {
		const float xRound = radius * 2.0f / (float)dim.x;
		const float yRound = radius * 2.0f / (float)dim.y;
		if (xRound > 0.0f && yRound > 0.0f) {
			center.x = glm::round(center.x / xRound) * xRound;
			center.y = glm::round(center.y / yRound) * yRound;
		}
	}

	const float zPad = core_max(32.0f, core_max(extent.z, radius) * 0.25f);
	float zNear = -lightMaxs.z - zPad;
	float zFar = -lightMins.z + 16.0f;
	if (zFar < zNear + 1.0f) {
		zFar = zNear + 1.0f;
	}
	const glm::mat4 lightProjection = video::clipDepthZeroToOne()
		? glm::orthoRH_ZO(center.x - radius, center.x + radius, center.y - radius, center.y + radius, zNear, zFar)
		: glm::ortho(center.x - radius, center.x + radius, center.y - radius, center.y + radius, zNear, zFar);
	return lightProjection * lightView;
}

} // namespace

void Shadow::update(const video::Camera& camera, bool active, const glm::vec3& sceneMins, const glm::vec3& sceneMaxs) {
	core_trace_scoped(ShadowCalculate);

	if (!active) {
		for (int i = 0; i < _parameters.maxDepthBuffers; ++i) {
			_cascades[i] = glm::mat4(1.0f);
			_distances[i] = camera.farPlane();
		}
		return;
	}

	const bool hasScene = sceneMins.x < sceneMaxs.x && sceneMins.y < sceneMaxs.y && sceneMins.z < sceneMaxs.z;
	const glm::ivec2 &dim = dimension();
	float near = camera.isOrthographic() ? camera.nearPlane() : core_max(camera.nearPlane(), 0.01f);
	float far = camera.farPlane();
	glm::vec3 sceneLightMins(0.0f);
	glm::vec3 sceneLightMaxs(0.0f);
	if (hasScene) {
		worldAABBToLightSpace(_lightView, sceneMins, sceneMaxs, sceneLightMins, sceneLightMaxs);
		glm::vec3 sceneViewMins;
		glm::vec3 sceneViewMaxs;
		worldAABBToLightSpace(camera.viewMatrix(), sceneMins, sceneMaxs, sceneViewMins, sceneViewMaxs);
		const float sceneNear = core_max(near, -sceneViewMaxs.z);
		const float sceneFar = core_min(far, -sceneViewMins.z);
		if (sceneNear < sceneFar) {
			near = sceneNear;
			far = sceneFar;
		}
	}

	float sliceNear = near;
	for (int i = 0; i < _parameters.maxDepthBuffers; ++i) {
		const float fraction = (float)(i + 1) / (float)_parameters.maxDepthBuffers;
		float sliceFar = glm::mix(near, far, fraction);
		if (!camera.isOrthographic()) {
			const float logarithmic = near * glm::pow(far / near, fraction);
			sliceFar = glm::mix(sliceFar, logarithmic, glm::clamp(_parameters.sliceWeight, 0.0f, 1.0f));
		}
		glm::vec3 corners[math::FRUSTUM_VERTICES_MAX];
		camera.splitFrustum(sliceNear, sliceFar, corners);
		glm::vec3 lightMins;
		glm::vec3 lightMaxs;
		lightSpaceAABB(_lightView, corners, math::FRUSTUM_VERTICES_MAX, lightMins, lightMaxs);
		if (hasScene) {
			const glm::vec2 clippedMins = glm::max(glm::vec2(lightMins), glm::vec2(sceneLightMins));
			const glm::vec2 clippedMaxs = glm::min(glm::vec2(lightMaxs), glm::vec2(sceneLightMaxs));
			if (clippedMins.x < clippedMaxs.x && clippedMins.y < clippedMaxs.y) {
				lightMins.x = clippedMins.x;
				lightMins.y = clippedMins.y;
				lightMaxs.x = clippedMaxs.x;
				lightMaxs.y = clippedMaxs.y;
			} else {
				lightMins = sceneLightMins;
				lightMaxs = sceneLightMaxs;
			}
			// Keep upstream casters even when they lie outside the camera slice.
			lightMins.z = sceneLightMins.z;
			lightMaxs.z = sceneLightMaxs.z;
		}
		_cascades[i] = lightOrtho(_lightView, lightMins, lightMaxs, dim);
		_distances[i] = sliceFar;
		sliceNear = sliceFar;
	}
}

bool Shadow::bind(video::TextureUnit unit) {
	const bool state = video::bindTexture(unit, _depthBuffer, video::FrameBufferAttachment::Depth);
	core_assert(state);
	return state;
}

void Shadow::render(const funcRender& renderCallback, bool clearDepthBuffer) {
	core_trace_scoped(ShadowPass);
	video_trace_scoped(ShadowRender);
	const bool oldBlend = video::disable(video::State::Blend);
	// Greedy voxel meshes are single-sided. Front-face culling wrote only the
	// sides away from the sun, so casters self-shadowed and cast a back-face hull.
	video::enable(video::State::CullFace);
	video::cullFace(video::Face::Back);

	video::enable(video::State::PolygonOffsetFill);
	video::polygonOffset(glm::vec2(1.0f, 2.0f));

	video::colorMask(false, false, false, false);
	_depthBuffer.bind(false);
	for (int i = 0; i < _parameters.maxDepthBuffers; ++i) {
		core_trace_value_scoped(ShadowCascade, i);
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
	_sunDirection = glm::vec3(glm::column(glm::inverse(_lightView), 2));
}

glm::vec3 Shadow::sunPosition() const {
	const glm::mat3 rotMat(_lightView);
	const glm::vec3 d(_lightView[3]);
	return -d * rotMat;
}

}
