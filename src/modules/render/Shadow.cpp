/**
 * @file
 */

#include "Shadow.h"
#include "video/Camera.h"
#include "video/Shader.h"
#include "video/Texture.h"
#include "core/GLM.h"
#include "core/Trace.h"
#include "core/Var.h"
#include "video/ScopedViewPort.h"
#include "video/ScopedPolygonMode.h"
#include "video/Buffer.h"
#include "video/Renderer.h"

namespace render {

Shadow::Shadow() :
		_shadowMapShader(shader::ShadowmapShader::getInstance()),
		_shadowMapRenderShader(shader::ShadowmapRenderShader::getInstance()),
		_shadowMapInstancedShader(shader::ShadowmapInstancedShader::getInstance()) {
}

Shadow::~Shadow() {
	core_assert_msg(_parameters.maxDepthBuffers == -1, "Shadow::shutdown() wasn't called");
}

bool Shadow::init(const ShadowParameters& parameters) {
	_parameters = parameters;
	core_assert(_parameters.maxDepthBuffers >= 1);
	const float length = 50.0f;
	const glm::vec3 sunPos(length, length, -length);
	setPosition(sunPos);
	if (!_shadowMapShader.setup()) {
		Log::error("Failed to init shadowmap shader");
		return false;
	}
	if (!_shadowMapRenderShader.setup()) {
		Log::error("Failed to init shadowmap debug shader");
		return false;
	}
	if (!_shadowMapInstancedShader.setup()) {
		Log::error("Failed to init shadowmap instanced debug shader");
		return false;
	}

	const glm::ivec2 smSize(core::Var::getSafe(cfg::ClientShadowMapSize)->intVal());
	const video::FrameBufferConfig& cfg = video::defaultDepthBufferConfig(smSize, _parameters.maxDepthBuffers);
	if (!_depthBuffer.init(cfg)) {
		Log::error("Failed to init the depthbuffer");
		return false;
	}

	const glm::ivec2& fullscreenQuadIndices = _shadowMapDebugBuffer.createFullscreenTexturedQuad(true);
	_shadowMapDebugBuffer.addAttribute(_shadowMapRenderShader.getPosAttribute(fullscreenQuadIndices.x, &glm::vec3::x));
	_shadowMapDebugBuffer.addAttribute(_shadowMapRenderShader.getTexcoordAttribute(fullscreenQuadIndices.y, &glm::vec2::x));

	return true;
}

void Shadow::shutdown() {
	_shadowMapDebugBuffer.shutdown();
	_shadowMapRenderShader.shutdown();
	_depthBuffer.shutdown();
	_shadowMapShader.shutdown();
	_shadowMapInstancedShader.shutdown();
	_parameters = ShadowParameters();
}

void Shadow::update(const video::Camera& camera, bool active) {
	core_trace_scoped(ShadowCalculate);
	_shadowRangeZ = camera.farPlane() * 3.0f;
	_cascades.resize(_parameters.maxDepthBuffers);
	_distances.resize(_parameters.maxDepthBuffers);
	const glm::ivec2& dim = dimension();

	if (!active) {
		for (int i = 0; i < _parameters.maxDepthBuffers; ++i) {
			_cascades[i] = glm::mat4(1.0f);
			_distances[i] = camera.farPlane();
		}
		return;
	}

	std::vector<float> planes(_parameters.maxDepthBuffers * 2);
	camera.sliceFrustum(&planes.front(), planes.size(), _parameters.maxDepthBuffers, _parameters.sliceWeight);
	const glm::mat4& inverseView = camera.inverseViewMatrix();

	for (int i = 0; i < _parameters.maxDepthBuffers; ++i) {
		const float near = planes[i * 2 + 0];
		const float far = planes[i * 2 + 1];
		const glm::vec4& sphere = camera.splitFrustumSphereBoundingBox(near, far);
		const glm::vec3 lightCenter(_lightView * inverseView * glm::vec4(sphere.x, sphere.y, sphere.z, 1.0f));
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

void Shadow::render(funcRender renderCallback, funcRenderInstance renderInstancedCallback) {
	const bool oldBlend = video::disable(video::State::Blend);
	// put shadow acne into the dark
	const bool cullFaceChanged = video::cullFace(video::Face::Front);
	const glm::vec2 offset(_parameters.shadowBiasSlope, (_parameters.shadowBias / _shadowRangeZ) * (1 << 24));
	const video::ScopedPolygonMode scopedPolygonMode(video::PolygonMode::Solid, offset);

	_depthBuffer.bind(false);
	for (int i = 0; i < _parameters.maxDepthBuffers; ++i) {
		_depthBuffer.bindTextureAttachment(video::FrameBufferAttachment::Depth, i, true);
		{
			video::ScopedShader scoped(_shadowMapShader);
			_shadowMapShader.setLightviewprojection(_cascades[i]);
			if (!renderCallback(i, _shadowMapShader)) {
				break;
			}
		}
		{
			video::ScopedShader scoped(_shadowMapInstancedShader);
			_shadowMapInstancedShader.setLightviewprojection(_cascades[i]);
			if (!renderInstancedCallback(i, _shadowMapInstancedShader)) {
				break;
			}
		}
	}
	_depthBuffer.unbind();
	if (cullFaceChanged) {
		video::cullFace(video::Face::Back);
	}
	if (oldBlend) {
		video::enable(video::State::Blend);
	}
}

void Shadow::renderShadowMap(const video::Camera& camera) {
	core_trace_scoped(RenderShadowMap);
	const int frameBufferWidth = camera.frameBufferWidth();
	const int frameBufferHeight = camera.frameBufferHeight();

	// activate shader
	video::ScopedShader scopedShader(_shadowMapRenderShader);
	_shadowMapRenderShader.recordUsedUniforms(true);
	_shadowMapRenderShader.clearUsedUniforms();
	_shadowMapRenderShader.setShadowmap(video::TextureUnit::Zero);
	_shadowMapRenderShader.setFar(camera.farPlane());
	_shadowMapRenderShader.setNear(camera.nearPlane());

	// bind buffers
	video::ScopedBuffer scopedBuf(_shadowMapDebugBuffer);

	// configure shadow map texture
	const video::TexturePtr& depthTex = _depthBuffer.texture(video::FrameBufferAttachment::Depth);
	video::bindTexture(video::TextureUnit::Zero, depthTex);
	video::setupDepthCompareTexture(depthTex->type(), video::CompareFunc::Less, video::TextureCompareMode::None);

	// render shadow maps
	for (int i = 0; i < _parameters.maxDepthBuffers; ++i) {
		const int halfWidth = (int) (frameBufferWidth / 4.0f);
		const int halfHeight = (int) (frameBufferHeight / 4.0f);
		video::ScopedViewPort scopedViewport(i * halfWidth, 0, halfWidth, halfHeight);
		_shadowMapRenderShader.setCascade(i);
		video::drawArrays(video::Primitive::Triangles, _shadowMapDebugBuffer.elements(0));
	}

	// restore texture
	video::setupDepthCompareTexture(depthTex->type(), video::CompareFunc::Less, video::TextureCompareMode::RefToTexture);
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
