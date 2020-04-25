/**
 * @file
 */

#include "WorldRenderer.h"
#include "WorldShaderConstants.h"

#include "core/Color.h"
#include "core/GameConfig.h"
#include "core/ArrayLength.h"
#include "core/Assert.h"
#include "core/Var.h"
#include "video/Trace.h"
#include "core/GLM.h"
#include "voxel/Constants.h"
#include "voxel/MaterialColor.h"

#include "frontend/Colors.h"

#include "video/Renderer.h"
#include "video/ScopedState.h"
#include "video/Types.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

namespace voxelrender {

WorldRenderer::WorldRenderer() :
		_threadPool(2, "WorldRenderer"), _worldChunkMgr(_threadPool), _shadowMapShader(shader::ShadowmapShader::getInstance()) {
	setViewDistance(240.0f);
}

WorldRenderer::~WorldRenderer() {
}

void WorldRenderer::reset() {
	_worldChunkMgr.reset();
	_entityMgr.reset();
}

void WorldRenderer::shutdown() {
	_cancelThreads = true;
	_worldShader.shutdown();
	_waterShader.shutdown();
	_materialBlock.shutdown();
	_entityRenderer.shutdown();
	reset();
	_worldChunkMgr.shutdown();
	_colorTexture.shutdown();
	if (_distortionTexture) {
		_distortionTexture->shutdown();
	}
	if (_normalTexture) {
		_normalTexture->shutdown();
	}
	_worldBuffers.shutdown();
	_shadow.shutdown();
	_skybox.shutdown();
	_shadowMapShader.shutdown();
	shutdownFrameBuffers();
	_postProcessBuf.shutdown();
	_postProcessBufId = -1;
	_postProcessShader.shutdown();
	_threadPool.shutdown();
}

int WorldRenderer::renderWorld(const video::Camera& camera) {
	core_trace_scoped(WorldRendererRenderWorld);
	int drawCallsWorld = 0;
	drawCallsWorld += renderToFrameBuffer(camera);
	drawCallsWorld += renderPostProcessEffects(camera);
	return drawCallsWorld;
}

int WorldRenderer::renderPostProcessEffects(const video::Camera& camera) {
	video_trace_scoped(WorldRendererRenderPostProcess);
	video::ScopedState depthTest(video::State::DepthTest, false);
	const video::TexturePtr& fboTexture = _frameBuffer.texture(video::FrameBufferAttachment::Color0);
	video::ScopedShader scoped(_postProcessShader);
	video::ScopedTexture scopedTex(fboTexture, video::TextureUnit::Zero);
	video::ScopedBuffer scopedBuf(_postProcessBuf);
	const int currentEyeHeight = (int)camera.eye().y;
	if (currentEyeHeight <= voxel::MAX_WATER_HEIGHT) {
		static const voxel::Voxel waterVoxel = voxel::createColorVoxel(voxel::VoxelType::Water, 0);
		const glm::vec4& waterColor = voxel::getMaterialColor(waterVoxel);
		_postProcessShader.setColor(waterColor);
	} else {
		_postProcessShader.setColor(glm::one<glm::vec4>());
	}
	_postProcessShader.setTexture(video::TextureUnit::Zero);
	const int elements = _postProcessBuf.elements(_postProcessBufId, _postProcessShader.getComponentsPos());
	video::drawArrays(video::Primitive::Triangles, elements);
	return 1;
}

glm::mat4 WorldRenderer::waterModelMatrix() const {
	constexpr glm::vec3 translate(0.0f, ((float)voxel::MAX_WATER_HEIGHT) - 0.1f, 0.0f);
	const glm::mat4& model = glm::scale(glm::translate(glm::mat4(1.0f), translate), glm::vec3(1000.0f));
	return model;
}

// http://www.bcnine.com/articles/water/water.md.html
glm::mat4 WorldRenderer::reflectionMatrix(const video::Camera& camera) const {
	constexpr float waterHeight = (float)voxel::MAX_WATER_HEIGHT;
	constexpr glm::mat4 reflection(
			1.0, 0.0, 0.0, 0.0,
			0.0, -1.0, 0.0, 0.0,
			0.0, 0.0, 1.0, 0.0,
			0.0, 2.0 * waterHeight, 0.0, 1.0);
	constexpr glm::mat4 flip(
			1.0, 0.0, 0.0, 0.0,
			0.0, -1.0, 0.0, 0.0,
			0.0, 0.0, 1.0, 0.0,
			0.0, 0.0, 0.0, 1.0);
	// TODO: increase the field-of-view for the reflection camera a little bit to reduce artifacts on the
	// edges of the water a little bit - e.g. Apply a 1.05 factor to the current FOV
	return camera.projectionMatrix() * glm::inverse(reflection * camera.inverseViewMatrix() * flip);
}

int WorldRenderer::renderClippingPlanes(const video::Camera& camera) {
	core_trace_scoped(WorldRendererRenderClippingPlane);
	constexpr float waterHeight = (float)voxel::MAX_WATER_HEIGHT;
	// apply a small bias to improve reflections of objects on the water when the
	// reflections are distorted.
	constexpr glm::vec4 waterAbovePlane(glm::up, -waterHeight);
	constexpr glm::vec4 waterBelowPlane(glm::down, waterHeight);

	int drawCallsWorld = 0;
	video::ScopedState scopedClipDistance(video::State::ClipDistance, true);

	// render above water
	_reflectionBuffer.bind(true);
	const glm::mat4& vpmatRefl = reflectionMatrix(camera);
	drawCallsWorld += renderTerrain(vpmatRefl, waterAbovePlane);
	drawCallsWorld += renderEntities(vpmatRefl, waterAbovePlane);
	_reflectionBuffer.unbind();

	// render below water
	const glm::mat4& vpmat = camera.viewProjectionMatrix();
	_refractionBuffer.bind(true);
	drawCallsWorld += renderTerrain(vpmat, waterBelowPlane);
	drawCallsWorld += renderEntities(vpmat, waterBelowPlane);
	_refractionBuffer.unbind();

	return drawCallsWorld;
}

int WorldRenderer::renderToShadowMap(const video::Camera& camera) {
	if (!_shadowMap->boolVal()) {
		return 0;
	}
	core_trace_scoped(WorldRendererRenderShadow);

	// render the entities
	_entityRenderer.renderShadows(_entityMgr.visibleEntities(), _shadow);

	// render the terrain
	_shadowMapShader.activate();
	_shadowMapShader.setModel(glm::mat4(1.0f));
	_shadow.render([this] (int i, const glm::mat4& lightViewProjection) {
		_shadowMapShader.setLightviewprojection(lightViewProjection);
		_worldChunkMgr.renderTerrain();
		return true;
	}, false);
	_shadowMapShader.deactivate();
	return (int)_entityMgr.visibleEntities().size() + 1;
}

int WorldRenderer::renderToFrameBuffer(const video::Camera& camera) {
	core_trace_scoped(WorldRendererRenderToFrameBuffer);

	// ensure we are in the expected states
	video::enable(video::State::DepthTest);
	video::depthFunc(video::CompareFunc::LessEqual);
	video::enable(video::State::CullFace);
	video::cullFace(video::Face::Back);
	video::enable(video::State::DepthMask);
	video::clearColor(frontend::clearColor);

	int drawCallsWorld = 0;

	// render depth buffers
	drawCallsWorld += renderEntitiesToDepthMap(camera);
	drawCallsWorld += renderToShadowMap(camera);

	// bind texture units
	_shadow.bind(video::TextureUnit::One);
	_colorTexture.bind(video::TextureUnit::Zero);

	// render reflection and refraction buffers
	drawCallsWorld += renderClippingPlanes(camera);

	// render everything into the framebuffer
	_frameBuffer.bind(true);
	drawCallsWorld += renderAll(camera);

	// cleanup
	video::bindVertexArray(video::InvalidId);
	_colorTexture.unbind();
	_frameBuffer.unbind();

	return drawCallsWorld;
}

int WorldRenderer::renderTerrain(const glm::mat4& viewProjectionMatrix, const glm::vec4& clipPlane) {
	int drawCallsWorld = 0;
	video_trace_scoped(WorldRendererRenderOpaque);
	video::ScopedShader scoped(_worldShader);
	if (_worldShader.isDirty()) {
		_worldShader.setFogcolor(frontend::clearColor);
		_worldShader.setMaterialblock(_materialBlock);
		_worldShader.setModel(glm::mat4(1.0f));
		_worldShader.setTexture(video::TextureUnit::Zero);
		_worldShader.setDiffuseColor(frontend::diffuseColor);
		_worldShader.setAmbientColor(frontend::ambientColor);
		_worldShader.setNightColor(frontend::nightColor);
		_worldShader.setShadowmap(video::TextureUnit::One);
		_worldShader.setEntitiesdepthmap(video::TextureUnit::Two);
		_worldShader.markClean();
	}
	_worldShader.setFocuspos(_focusPos);
	_worldShader.setLightdir(_shadow.sunDirection());
	_worldShader.setTime(_seconds);
	_worldShader.setFogrange(_fogRange);
	_worldShader.setClipplane(clipPlane);
	_worldShader.setViewprojection(viewProjectionMatrix);
	_entityRenderer.bindEntitiesDepthBuffer(video::TextureUnit::Two);
	if (_shadowMap->boolVal()) {
		_worldShader.setDepthsize(glm::vec2(_shadow.dimension()));
		_worldShader.setCascades(_shadow.cascades());
		_worldShader.setDistances(_shadow.distances());
	}
	drawCallsWorld += _worldChunkMgr.renderTerrain();
	return drawCallsWorld;
}

int WorldRenderer::renderWater(const video::Camera& camera, const glm::vec4& clipPlane) {
	int drawCallsWorld = 0;
	video_trace_scoped(WorldRendererRenderWater);
	video::ScopedShader scoped(_waterShader);
	if (_waterShader.isDirty()) {
		_waterShader.setModel(waterModelMatrix());
		_waterShader.setShadowmap(video::TextureUnit::One);
		_waterShader.setCubemap(video::TextureUnit::Two);
		_waterShader.setReflection(video::TextureUnit::Three);
		_waterShader.setRefraction(video::TextureUnit::Four);
		_waterShader.setDistortion(video::TextureUnit::Five);
		_waterShader.setNormalmap(video::TextureUnit::Six);
		_waterShader.setDepthmap(video::TextureUnit::Seven);
		_waterShader.setFogcolor(frontend::clearColor);
		_waterShader.setDiffuseColor(frontend::diffuseColor);
		_waterShader.setAmbientColor(frontend::ambientColor);
		_waterShader.setNightColor(frontend::nightColor);
		_waterShader.markClean();
	}
	_waterShader.setFocuspos(_focusPos);
	_waterShader.setCamerapos(camera.position());
	_waterShader.setLightdir(_shadow.sunDirection());
	_waterShader.setFogrange(_fogRange);
	_waterShader.setTime(_seconds);
	_waterShader.setFar(camera.farPlane());
	_waterShader.setNear(camera.nearPlane());
	_skybox.bind(video::TextureUnit::Two);
	_reflectionBuffer.texture()->bind(video::TextureUnit::Three);
	_refractionBuffer.texture()->bind(video::TextureUnit::Four);
	_distortionTexture->bind(video::TextureUnit::Five);
	_normalTexture->bind(video::TextureUnit::Six);
	_refractionBuffer.texture(video::FrameBufferAttachment::Depth)->bind(video::TextureUnit::Seven);
	_waterShader.setViewprojection(camera.viewProjectionMatrix());
	if (_shadowMap->boolVal()) {
		_waterShader.setDepthsize(glm::vec2(_shadow.dimension()));
		_waterShader.setCascades(_shadow.cascades());
		_waterShader.setDistances(_shadow.distances());
	}
	if (_worldBuffers.renderWater()) {
		++drawCallsWorld;
	}
	_skybox.unbind(video::TextureUnit::Two);
	_normalTexture->unbind();
	_distortionTexture->unbind();
	_refractionBuffer.texture()->unbind();
	_refractionBuffer.texture(video::FrameBufferAttachment::Depth)->unbind();
	_reflectionBuffer.texture()->unbind();
	return drawCallsWorld;
}

int WorldRenderer::renderAll(const video::Camera& camera) {
	core_trace_scoped(WorldRendererAll);
	int drawCallsWorld = 0;
	// due to driver bugs the clip plane might still be taken into account
	constexpr glm::vec4 ignoreClipPlane(glm::up, 0.0f);
	const glm::mat4& vpmat = camera.viewProjectionMatrix();
	drawCallsWorld += renderTerrain(vpmat, ignoreClipPlane);
	drawCallsWorld += renderEntities(vpmat, ignoreClipPlane);
	drawCallsWorld += renderEntityDetails(camera);
	drawCallsWorld += renderWater(camera, ignoreClipPlane);
	_skybox.render(camera);
	return drawCallsWorld;
}

int WorldRenderer::renderEntitiesToDepthMap(const video::Camera& camera) {
	return _entityRenderer.renderEntitiesToDepthMap(_entityMgr.visibleEntities(), camera.viewProjectionMatrix());
}

int WorldRenderer::renderEntities(const glm::mat4& viewProjectionMatrix, const glm::vec4& clipPlane) {
	return _entityRenderer.renderEntities(_entityMgr.visibleEntities(), viewProjectionMatrix, clipPlane, _shadow);
}

int WorldRenderer::renderEntityDetails(const video::Camera& camera) {
	return _entityRenderer.renderEntityDetails(_entityMgr.visibleEntities(), camera);
}

void WorldRenderer::construct() {
	_shadowMap = core::Var::getSafe(cfg::ClientShadowMap);
	_entityRenderer.construct();
}

bool WorldRenderer::init(voxel::PagedVolume* volume, const glm::ivec2& position, const glm::ivec2& dimension) {
	core_trace_scoped(WorldRendererOnInit);
	_threadPool.init();

	_colorTexture.init();

	_distortionTexture = video::createTextureFromImage("water-distortion.png");
	if (!_distortionTexture || !_distortionTexture->isLoaded()) {
		Log::error("Failed to load distortion texture");
		return false;
	}

	_normalTexture = video::createTextureFromImage("water-normal.png");
	if (!_normalTexture || !_normalTexture->isLoaded()) {
		Log::error("Failed to load normalmap texture");
		return false;
	}

	if (!_entityRenderer.init()) {
		Log::error("Failed to initialize the entity renderer");
		return false;
	}

	if (!_worldShader.setup()) {
		Log::error("Failed to setup the post world shader");
		return false;
	}
	if (!_waterShader.setup()) {
		Log::error("Failed to setup the post water shader");
		return false;
	}
	if (!_postProcessShader.setup()) {
		Log::error("Failed to setup the post processing shader");
		return false;
	}
	if (!_skybox.init("sky")) {
		Log::error("Failed to initialize the sky");
		return false;
	}
	if (!_shadowMapShader.setup()) {
		Log::error("Failed to init shadowmap shader");
		return false;
	}
	const int shaderMaterialColorsArraySize = lengthof(shader::WorldData::MaterialblockData::materialcolor);
	const int materialColorsArraySize = (int)voxel::getMaterialColors().size();
	if (shaderMaterialColorsArraySize != materialColorsArraySize) {
		Log::error("Shader parameters and material colors don't match in their size: %i - %i",
				shaderMaterialColorsArraySize, materialColorsArraySize);
		return false;
	}

	shader::WorldData::MaterialblockData materialBlock;
	memcpy(materialBlock.materialcolor, &voxel::getMaterialColors().front(), sizeof(materialBlock.materialcolor));
	_materialBlock.create(materialBlock);

	if (!_worldBuffers.init(_waterShader)) {
		return false;
	}

	render::ShadowParameters sp;
	sp.maxDepthBuffers = shader::WorldShaderConstants::getMaxDepthBuffers();
	if (!_shadow.init(sp)) {
		return false;
	}

	_worldChunkMgr.init(&_worldShader, volume);
	_worldChunkMgr.updateViewDistance(_viewDistance);
	_threadPool.enqueue([this] () {while (!_cancelThreads) { _worldChunkMgr.extractScheduledMesh(); } });

	if (!initFrameBuffers(dimension)) {
		return false;
	}
	_postProcessBufId = _postProcessBuf.createFullscreenTextureBufferYFlipped();
	if (_postProcessBufId == -1) {
		return false;
	}

	struct VertexFormat {
		constexpr VertexFormat(const glm::vec2& p, const glm::vec2& t) : pos(p), tex(t) {}
		glm::vec2 pos;
		glm::vec2 tex;
	};
	alignas(16) constexpr VertexFormat vecs[] = {
		// left bottom
		VertexFormat(glm::vec2(-1.0f, -1.0f), glm::vec2(0.0f)),
		// right bottom
		VertexFormat(glm::vec2( 1.0f, -1.0f), glm::vec2(1.0f, 0.0f)),
		// right top
		VertexFormat(glm::vec2( 1.0f,  1.0f), glm::vec2(1.0f)),
		// left bottom
		VertexFormat(glm::vec2(-1.0f, -1.0f), glm::vec2(0.0f)),
		// right top
		VertexFormat(glm::vec2( 1.0f,  1.0f), glm::vec2(1.0f)),
		// left top
		VertexFormat(glm::vec2(-1.0f,  1.0f), glm::vec2(0.0f, 1.0f)),
	};

	_postProcessBufId = _postProcessBuf.create(vecs, sizeof(vecs));
	_postProcessBuf.addAttribute(_postProcessShader.getPosAttribute(_postProcessBufId, &VertexFormat::pos));
	_postProcessBuf.addAttribute(_postProcessShader.getTexcoordAttribute(_postProcessBufId, &VertexFormat::tex));

	return true;
}

bool WorldRenderer::initFrameBuffers(const glm::ivec2& dimensions) {
	video::TextureConfig textureCfg;
	textureCfg.wrap(video::TextureWrap::ClampToEdge);
	textureCfg.format(video::TextureFormat::RGB);
	video::FrameBufferConfig cfg;
	cfg.dimension(dimensions).depthBuffer(true).depthBufferFormat(video::TextureFormat::D24);
	cfg.addTextureAttachment(textureCfg, video::FrameBufferAttachment::Color0);
	if (!_frameBuffer.init(cfg)) {
		Log::error("Failed to initialize the default frame buffer");
		return false;
	}

	textureCfg = video::TextureConfig();
	textureCfg.format(video::TextureFormat::RGB);
	video::FrameBufferConfig refractionCfg;
	refractionCfg.dimension(dimensions / 2).depthTexture(true).depthTextureFormat(video::TextureFormat::D32F);
	refractionCfg.addTextureAttachment(textureCfg, video::FrameBufferAttachment::Color0);
	if (!_refractionBuffer.init(refractionCfg)) {
		Log::error("Failed to initialize the refraction frame buffer");
		return false;
	}

	textureCfg = video::TextureConfig();
	textureCfg.format(video::TextureFormat::RGB);
	video::FrameBufferConfig reflectionCfg;
	reflectionCfg.dimension(dimensions);
	reflectionCfg.addTextureAttachment(textureCfg, video::FrameBufferAttachment::Color0);
	if (!_reflectionBuffer.init(reflectionCfg)) {
		Log::error("Failed to initialize the reflection frame buffer");
		return false;
	}

	return true;
}

void WorldRenderer::shutdownFrameBuffers() {
	_frameBuffer.shutdown();
	_refractionBuffer.shutdown();
	_reflectionBuffer.shutdown();
}

void WorldRenderer::update(const video::Camera& camera, uint64_t dt) {
	core_trace_scoped(WorldRendererOnRunning);
	_focusPos = camera.target();
	_focusPos.y = 0.0f;//TODO: _world->findFloor(_focusPos.x, _focusPos.z, voxel::isFloor);

	_shadow.update(camera, _shadowMap->boolVal());
	_entityRenderer.update(_focusPos, _seconds);

	_worldChunkMgr.update(camera, _focusPos);
	_entityMgr.update(dt);
	_entityMgr.updateVisibleEntities(dt, camera);
}

void WorldRenderer::extractMesh(const glm::ivec3 &pos) {
	if (_cancelThreads) {
		return;
	}
	_worldChunkMgr.extractMesh(pos);
}

void WorldRenderer::extractMeshes(const video::Camera &camera) {
	if (_cancelThreads) {
		return;
	}
	_worldChunkMgr.extractMeshes(camera);
}

}
