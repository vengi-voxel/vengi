/**
 * @file
 */

#include "Shadow.h"
#include "core/TraceScoped.h"
#include "math/Frustum.h"
#include "video/Camera.h"
#include "video/Trace.h"
#include "core/Var.h"
#include "core/Log.h"
#include "video/Renderer.h"
#include "math/Mat4f.h"
#include "math/Mat3f.h"
#include "math/Vec4f.h"
#include "math/Vec3f.h"
#include "math/Vec2i.h"
#include "math/Vec2f.h"
#include "math/Conversions.h"
#include "math/Mat4Ops.h"
#include "math/Axes.h"

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
	const math::Vec3f sunPos(25.0f, 100.0f, 25.0f);
	setPosition(sunPos, math::Vec3f(0.0f), math::up());

	const math::Vec2i smSize(core::getVar(cfg::ClientShadowMapSize)->intVal());
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

void lightSpaceAABB(const math::Mat4f &lightView, const math::Vec3f *worldPts, int n, math::Vec3f &mins, math::Vec3f &maxs) {
	mins = math::Vec3f(1.0e8f);
	maxs = math::Vec3f(-1.0e8f);
	for (int i = 0; i < n; ++i) {
		const math::Vec3f lp(lightView * math::Vec4f(worldPts[i], 1.0f));
		mins = math::min(mins, lp);
		maxs = math::max(maxs, lp);
	}
}

void worldAABBToLightSpace(const math::Mat4f &lightView, const math::Vec3f &wmins, const math::Vec3f &wmaxs, math::Vec3f &mins,
						   math::Vec3f &maxs) {
	math::Vec3f pts[8];
	for (int i = 0; i < 8; ++i) {
		pts[i] = math::Vec3f((i & 1) ? wmaxs.x : wmins.x, (i & 2) ? wmaxs.y : wmins.y, (i & 4) ? wmaxs.z : wmins.z);
	}
	lightSpaceAABB(lightView, pts, 8, mins, maxs);
}

math::Mat4f lightOrtho(const math::Mat4f &lightView, const math::Vec3f &lightMins, const math::Vec3f &lightMaxs,
					 const math::Vec2i &dim) {
	math::Vec3f center = (lightMins + lightMaxs) * 0.5f;
	const math::Vec3f extent = (lightMaxs - lightMins) * 0.5f;
	float radius = core_max(extent.x, extent.y);
	radius = core_max(radius, 8.0f);
	radius += 2.0f;

	if (dim.x > 0 && dim.y > 0) {
		const float xRound = radius * 2.0f / (float)dim.x;
		const float yRound = radius * 2.0f / (float)dim.y;
		if (xRound > 0.0f && yRound > 0.0f) {
			center.x = math::round(center.x / xRound) * xRound;
			center.y = math::round(center.y / yRound) * yRound;
		}
	}

	const float zPad = core_max(32.0f, core_max(extent.z, radius) * 0.25f);
	float zNear = -lightMaxs.z - zPad;
	float zFar = -lightMins.z + 16.0f;
	if (zFar < zNear + 1.0f) {
		zFar = zNear + 1.0f;
	}
	const math::Mat4f lightProjection = video::clipDepthZeroToOne()
		? math::orthoRH_ZO(center.x - radius, center.x + radius, center.y - radius, center.y + radius, zNear, zFar)
		: math::ortho(center.x - radius, center.x + radius, center.y - radius, center.y + radius, zNear, zFar);
	return lightProjection * lightView;
}

} // namespace

void Shadow::update(const video::Camera& camera, bool active, const math::Vec3f& sceneMins, const math::Vec3f& sceneMaxs) {
	core_trace_scoped(ShadowCalculate);

	if (!active) {
		for (int i = 0; i < _parameters.maxDepthBuffers; ++i) {
			_cascades[i] = math::Mat4f(1.0f);
			_distances[i] = camera.farPlane();
		}
		return;
	}

	const bool hasScene = sceneMins.x < sceneMaxs.x && sceneMins.y < sceneMaxs.y && sceneMins.z < sceneMaxs.z;
	const math::Vec2i &dim = dimension();
	float near = camera.isOrthographic() ? camera.nearPlane() : core_max(camera.nearPlane(), 0.01f);
	float far = camera.farPlane();
	math::Vec3f sceneLightMins(0.0f);
	math::Vec3f sceneLightMaxs(0.0f);
	if (hasScene) {
		worldAABBToLightSpace(_lightView, sceneMins, sceneMaxs, sceneLightMins, sceneLightMaxs);
		math::Vec3f sceneViewMins;
		math::Vec3f sceneViewMaxs;
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
		float sliceFar = math::mix(near, far, fraction);
		if (!camera.isOrthographic()) {
			const float logarithmic = near * math::pow(far / near, fraction);
			sliceFar = math::mix(sliceFar, logarithmic, math::clamp(_parameters.sliceWeight, 0.0f, 1.0f));
		}
		math::Vec3f corners[math::FRUSTUM_VERTICES_MAX];
		camera.splitFrustum(sliceNear, sliceFar, corners);
		math::Vec3f lightMins;
		math::Vec3f lightMaxs;
		lightSpaceAABB(_lightView, corners, math::FRUSTUM_VERTICES_MAX, lightMins, lightMaxs);
		if (hasScene) {
			const math::Vec2f clippedMins = math::max(math::Vec2f(lightMins), math::Vec2f(sceneLightMins));
			const math::Vec2f clippedMaxs = math::min(math::Vec2f(lightMaxs), math::Vec2f(sceneLightMaxs));
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
	video::polygonOffset(math::Vec2f(1.0f, 2.0f));

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
	video::polygonOffset(math::Vec2f(0.0f));
	video::disable(video::State::PolygonOffsetFill);

	video::cullFace(video::Face::Back);
	if (oldBlend) {
		video::enable(video::State::Blend);
	}
}

const math::Vec2i& Shadow::dimension() const {
	return _depthBuffer.dimension();
}

void Shadow::setPosition(const math::Vec3f& eye, const math::Vec3f& center, const math::Vec3f& up) {
	setLightViewMatrix(math::lookAt(eye, center, up));
}

void Shadow::setLightViewMatrix(const math::Mat4f& lightView) {
	_lightView = lightView;
	_sunDirection = math::Vec3f(math::column(math::inverse(_lightView), 2));
}

math::Vec3f Shadow::sunPosition() const {
	const math::Mat3f rotMat(_lightView);
	const math::Vec3f d(_lightView[3]);
	return -d * rotMat;
}

}
